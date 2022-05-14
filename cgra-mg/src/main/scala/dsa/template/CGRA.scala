package dsa

import chisel3._
import chisel3.util._
import scala.collection.mutable
import scala.collection.mutable.{ArrayBuffer, ListBuffer}
import op._
import ir._

/** CGRA Top module
 * 
 * @param attrs     module attributes
 * @param dumpIR    if dump IR file
 */ 
class CGRA(attrs: mutable.Map[String, Any], dumpIR: Boolean) extends Module with IR{
  apply(attrs)
  apply("top_module", "CGRA")
  val rows = getAttrValue("num_row").asInstanceOf[Int]     // PE number in a row
  val cols = getAttrValue("num_colum").asInstanceOf[Int]   // PE number in a colum
  val dataWidth = getAttrValue("data_width").asInstanceOf[Int] // data width in bit
  // cfgParams
  val cfgDataWidth = getAttrValue("cfg_data_width").asInstanceOf[Int]
  val cfgAddrWidth = getAttrValue("cfg_addr_width").asInstanceOf[Int] 
  val cfgBlkOffset = getAttrValue("cfg_blk_offset").asInstanceOf[Int]   // configuration offset bit of blocks

  // ====== GPE-Specific attributes =======//
  // number of registers in Regfile
  val numRegRF = getAttrValue("num_rf_reg").asInstanceOf[Int]
  // supported operations
  val opsStr = getAttrValue("operations").asInstanceOf[ListBuffer[String]]
  val ops = opsStr.map(OPC.withName(_))
  val aluOperandNum = ops.map(OpInfo.getOperandNum(_)).max
  // max delay cycles of the DelayPipe
  val maxDelay = getAttrValue("max_delay").asInstanceOf[Int]
  
  // ====== GIB-Specific attributes =======//
  val numTrack = getAttrValue("num_track").asInstanceOf[Int]
  // trackRegedMode, 0: no reg; 1: half of GIBs reged; 2: all GIBs reged
  val trackRegedMode = getAttrValue("track_reged_mode").asInstanceOf[Int]
  // fcList
  val fcMap = getAttrValue("connect_flexibility").asInstanceOf[mutable.Map[String, Int]]
  // val fci  = fcMap("num_itrack_per_ipin")     // ipin-itrack connection flexibility, connected track number
  // val fco  = fcMap("num_otrack_per_opin")     // opin-otrack connection flexibility, connected track number
  // val fcio = fcMap("num_ipin_per_opin")       // opin-ipin  connection flexibility, connected ipin number
  // if support diagonal connections between OPins and IPins
  val diagPinConect = getAttrValue("diag_iopin_connect").asInstanceOf[Boolean]

  // ====== IOB-Specific attributes =======//
  val numInOB = getAttrValue("num_input_ob").asInstanceOf[Int] 
  val numOutIB = getAttrValue("num_output_ib").asInstanceOf[Int] // should be even
  val numOutOB = getAttrValue("num_output_ob").asInstanceOf[Int]
  val numInIB = getAttrValue("num_input_ib").asInstanceOf[Int]

  val numIn = 2*cols*numInIB
  val numOut = 2*cols*numOutOB
  apply("num_input", numIn)
  apply("num_output", numOut)

  val io = IO(new Bundle{
    val cfg_en   = Input(Bool())
    val cfg_addr = Input(UInt(cfgAddrWidth.W))
    val cfg_data = Input(UInt(cfgDataWidth.W))
    val en  = Input(Vec(cols, Bool())) // top and buttom row
    val in  = Input(Vec(numIn, UInt(dataWidth.W)))
    val out = Output(Vec(numOut, UInt(dataWidth.W)))
  })

  val gpe_attrs: mutable.Map[String, Any] = mutable.Map(
    "data_width" -> dataWidth,
    "cfg_data_width" -> cfgDataWidth,
    "cfg_addr_width" -> cfgAddrWidth,
    "cfg_blk_index" -> 0,
    "cfg_blk_offset" -> cfgBlkOffset,
    "x" -> 0,
    "y" -> 0,
    "num_rf_reg" -> numRegRF,
    "operations" -> opsStr,
    "num_input_per_operand" -> ListBuffer.fill(aluOperandNum){4},
    "max_delay" -> maxDelay
  )

  val gib_attrs: mutable.Map[String, Any] = mutable.Map(
    "data_width" -> dataWidth,
    "cfg_data_width" -> cfgDataWidth,
    "cfg_addr_width" -> cfgAddrWidth,
    "cfg_blk_index" -> 0,
    "cfg_blk_offset" -> cfgBlkOffset,
    "x" -> 0,
    "y" -> 0,
    "num_track" -> numTrack,
    "diag_iopin_connect" -> diagPinConect,
    "num_iopin_list" -> mutable.Map[String, Int](),
    "connect_flexibility" -> fcMap,
	  "track_reged" -> false
  )

  val iob_attrs: mutable.Map[String, Any] = mutable.Map(
    "data_width" -> dataWidth,
    "cfg_data_width" -> cfgDataWidth,
    "cfg_addr_width" -> cfgAddrWidth,
    "cfg_blk_index" -> 0,
    "cfg_blk_offset" -> cfgBlkOffset,
    "x" -> 0,
    "y" -> 0,
    "num_input" -> numInOB,
    "num_output" -> numOutIB
  )

  // ======= sub_modules attribute ========//
  // 1-n : sub-modules 
  val sm_id: mutable.Map[String, ListBuffer[Int]] = mutable.Map(
    "IB" -> ListBuffer[Int](),  
    "OB" -> ListBuffer[Int](),  
    "GPE" -> ListBuffer[Int](), 
    "GIB" -> ListBuffer[Int]()  
  )

  // ======= sub_module instances attribute ========//
  // 0 : this module
  // 1-n : sub-module instances 
  val smi_id: mutable.Map[String, ListBuffer[Int]] = mutable.Map(
    "This" -> ListBuffer(0),
    "IB" -> ListBuffer[Int](),  // id = cfg_blk_idx
    "OB" -> ListBuffer[Int](),  // id = cfg_blk_idx
    "GPE" -> ListBuffer[Int](), // id = cfg_blk_idx
    "GIB" -> ListBuffer[Int]()  // id = cfg_blk_idx
  )

  // sub-module id to attribute
  val sm_id_attrs = mutable.Map[Int, mutable.Map[String, Any]]()
  // sub-module instance id to attribute
  val smi_id_attrs = mutable.Map[Int, mutable.Map[String, Any]]()

  val ibs = new ArrayBuffer[IOB]()
  val obs = new ArrayBuffer[IOB]()
  val pes = new ArrayBuffer[GPE]()
  val gibs = new ArrayBuffer[GIB]()

  var sm_id_offset = 1
  // top and buttom row, IB and OB are put together
  // IB (0, 2R+3)
  for(k <- 0 until 2) { // top and buttom row
    val x = k*(2*rows+3)
    val iob_index_base = x*(cols+1)
    for(i <- 0 until cols){
      val y = 2*i+1
      val index = iob_index_base+i+1
      iob_attrs("cfg_blk_index") = index
      iob_attrs("x") = x
      iob_attrs("y") = y
      iob_attrs("num_input") = numInIB // connect to top input
      iob_attrs("num_output") = numOutIB // connect to GIB
      ibs += Module(new IOB(iob_attrs))
      val smi_id_attr: mutable.Map[String, Any] = mutable.Map(
        "module_id" -> sm_id_offset,
        "cfg_blk_index" -> index,
        "x" -> x,
        "y" -> y
      )
      smi_id("IB") += index
      smi_id_attrs += index -> smi_id_attr
    }
  }
  sm_id("IB") += sm_id_offset
  sm_id_attrs += sm_id_offset -> ibs.last.getAttrs

  // OB (1, 2R+4)
  sm_id_offset += 1
  for(k <- 0 until 2) { // top and buttom row
    val x = k*(2*rows+3)+1
    val iob_index_base = x*(cols+1)
    for (i <- 0 until cols) {
      val y = 2*i+1
      val index = iob_index_base+i+1
      iob_attrs("cfg_blk_index") = index
      iob_attrs("x") = x
      iob_attrs("y") = y
      iob_attrs("num_input") = numInOB // connect to GIB
      iob_attrs("num_output") = numOutOB // connect to top output
      obs += Module(new IOB(iob_attrs))
      val smi_id_attr: mutable.Map[String, Any] = mutable.Map(
        "module_id" -> sm_id_offset,
        "cfg_blk_index" -> index,
        "x" -> x,
        "y" -> y
      )
      smi_id("OB") += index
      smi_id_attrs += index -> smi_id_attr
    }
  }
  sm_id("OB") += sm_id_offset
  sm_id_attrs += sm_id_offset -> obs.last.getAttrs

  // GPE: 2*(1,2,...,rows)+1 row
  sm_id_offset += 1
  for(i <- 0 until rows){
    val x = 2*i+3
    for(j <- 0 until cols){
      val y = 2*j+1
      val index = x*(cols+1) + j + 1
      gpe_attrs("cfg_blk_index") = index
      gpe_attrs("x") = x
      gpe_attrs("y") = y
      pes += Module(new GPE(gpe_attrs))
      val smi_id_attr: mutable.Map[String, Any] = mutable.Map(
        "module_id" -> sm_id_offset,
        "cfg_blk_index" -> index,
        "x" -> x,
        "y" -> y
      )
      smi_id("GPE") += index
      smi_id_attrs += index -> smi_id_attr
    }
  }
  sm_id("GPE") += sm_id_offset
  sm_id_attrs += sm_id_offset -> pes.last.getAttrs

  // GIB: 2*(1,...,rows+1) row
  sm_id_offset += 1
  val iopin_list_map = mutable.Map[mutable.Map[String, Int], Int]()
  for(i <- 0 to rows){
    for(j <- 0 to cols){
      val num_iopin_list = mutable.Map[String, Int]()
      num_iopin_list += "ipin_nw" -> {
        if(i == 0 && j > 0) numInOB/2
        else if(i > 0 && j > 0) aluOperandNum
        else 0
      }
      num_iopin_list += "opin_nw" -> {
        if(i == 0 && j > 0) numOutIB
        else if(i > 0 && j > 0) 1
        else 0
      }
      num_iopin_list += "ipin_ne" -> {
        if(i == 0 && j < cols) numInOB/2
        else if(i > 0 && j < cols) aluOperandNum
        else 0
      }
      num_iopin_list += "opin_ne" -> {
        if(i == 0 && j < cols) numOutIB
        else if(i > 0 && j < cols) 1
        else 0
      }
      num_iopin_list += "ipin_se" -> {
        if(i == rows && j < cols) numInOB/2
        else if(i < rows && j < cols) aluOperandNum
        else 0
      }
      num_iopin_list += "opin_se" -> {
        if(i == rows && j < cols) numOutIB
        else if(i < rows && j < cols) 1
        else 0
      }
      num_iopin_list += "ipin_sw" -> {
        if(i == rows && j > 0) numInOB/2
        else if(i < rows && j > 0) aluOperandNum
        else 0
      }
      num_iopin_list += "opin_sw" -> {
        if(i == rows && j > 0) numOutIB
        else if(i < rows && j > 0) 1
        else 0
      }
      val x = 2*i+2
      val y = 2*j
      val index = x*(cols+1) + j + 1
	    val reged = {
        if(trackRegedMode == 0) false
        else if(trackRegedMode == 2) true
        else (i%2 + j%2) == 1
      }
      gib_attrs("num_iopin_list") = num_iopin_list
      gib_attrs("cfg_blk_index") = index
      gib_attrs("x") = x
      gib_attrs("y") = y
	    gib_attrs("track_reged") = reged
      gibs += Module(new GIB(gib_attrs))
      if(!iopin_list_map.contains(num_iopin_list)){
        iopin_list_map += num_iopin_list -> sm_id_offset
        sm_id("GIB") += sm_id_offset
        sm_id_attrs += sm_id_offset -> gibs.last.getAttrs
        sm_id_offset += 1
      }
      val smi_id_attr: mutable.Map[String, Any] = mutable.Map(
        "module_id" -> iopin_list_map(num_iopin_list),
        "cfg_blk_index" -> index,
        "x" -> x,
        "y" -> y,
		    "track_reged" -> reged
      )
      smi_id("GIB") += index
      smi_id_attrs += index -> smi_id_attr
    }
  }

  val sub_modules = sm_id.map{case (name, ids) =>
    ids.map{id => mutable.Map(
      "id" -> id, 
      "type" -> name,
      "attributes" -> sm_id_attrs(id)
    )}
  }.flatten
  apply("sub_modules", sub_modules)

  val instances = smi_id.map{case (name, ids) =>
    ids.map{id => mutable.Map(
      "id" -> id, 
      "type" -> name) ++ 
      {if(name != "This") smi_id_attrs(id) else mutable.Map[String, Any]()}
    }
  }.flatten
  apply("instances", instances)


  // ======= connections attribute ========//
  apply("connection_format", ("src_id", "src_type", "src_out_idx", "dst_id", "dst_type", "dst_in_idx"))
  // This:src_out_idx is the input index
  // This:dst_in_idx is the output index
  val connections = ListBuffer[(Int, String, Int, Int, String, Int)]()
  val portNameMap = gibs(0).portNameMap
  val lastRowGibBaseIdx = rows*(cols+1)
  // IOB connections to top/buttom I/O and GIB
  ibs.zipWithIndex.map{ case (ib, i) => 
    // ib.io.en := io.en(i)
//    ib.io.in(0) := io.in(i)
//    connections.append((smi_id("This")(0), "This", i, smi_id("IB")(i), "IB", 0))
    ib.io.in.zipWithIndex.foreach{ case (in, j) =>
      val thisInIdx = i*numInIB+j
      ib.io.in(j) := io.in(thisInIdx)
      connections.append((smi_id("This")(0), "This", thisInIdx, smi_id("IB")(i), "IB", j))
    }
    ib.io.out.zipWithIndex.foreach{ case (out, j) =>
      if(i < cols){ // top row
        val gibIdx = i
        gibs(gibIdx).io.opinNE(j) := out
        gibs(gibIdx+1).io.opinNW(j) := out
        val index1 = gibs(gibIdx).iPortMap("opinNE" + j.toString)
        val index2 = gibs(gibIdx+1).iPortMap("opinNW" + j.toString)
        connections.append((smi_id("IB")(i), "IB", j, smi_id("GIB")(gibIdx), "GIB", index1))
        connections.append((smi_id("IB")(i), "IB", j, smi_id("GIB")(gibIdx+1), "GIB", index2))
      }else{ // buttom row
        val gibIdx = lastRowGibBaseIdx+i-cols
        gibs(gibIdx).io.opinSE(j) := out
        gibs(gibIdx+1).io.opinSW(j) := out
        val index1 = gibs(gibIdx).iPortMap("opinSE" + j.toString)
        val index2 = gibs(gibIdx+1).iPortMap("opinSW" + j.toString)
        connections.append((smi_id("IB")(i), "IB", j, smi_id("GIB")(gibIdx), "GIB", index1))
        connections.append((smi_id("IB")(i), "IB", j, smi_id("GIB")(gibIdx+1), "GIB", index2))
      }
    }  
  }
  obs.zipWithIndex.map{ case (ob, i) => 
    // ob.io.en := io.en(i)
//    io.out(i) := ob.io.out(0)
//    connections.append((smi_id("OB")(i), "OB", 0, smi_id("This")(0), "This", i))
    ob.io.out.zipWithIndex.foreach{ case (out, j) =>
      val thisOutIdx = i*numOutOB+j
      io.out(thisOutIdx) := ob.io.out(j)
      connections.append((smi_id("OB")(i), "OB", j, smi_id("This")(0), "This", thisOutIdx))
    }
    ob.io.in.zipWithIndex.map{ case (in, j) =>
      if(i < cols){ // top row
        val gibIdx = i
        if(j%2 == 0) {
          in := gibs(gibIdx).io.ipinNE(j/2)
          val index = gibs(gibIdx).oPortMap("ipinNE" + (j/2).toString)
          connections.append((smi_id("GIB")(gibIdx), "GIB", index, smi_id("OB")(i), "OB", j))
        } else {
          in := gibs(gibIdx+1).io.ipinNW(j/2)
          val index = gibs(gibIdx+1).oPortMap("ipinNW" + (j/2).toString)
          connections.append((smi_id("GIB")(gibIdx+1), "GIB", index, smi_id("OB")(i), "OB", j))
        }
      }else { // buttom row
        val gibIdx = lastRowGibBaseIdx+i-cols
        if(j%2 == 0) {
          in := gibs(gibIdx).io.ipinSE(j/2)
          val index = gibs(gibIdx).oPortMap("ipinSE" + (j/2).toString)
          connections.append((smi_id("GIB")(gibIdx), "GIB", index, smi_id("OB")(i), "OB", j))
        } else {
          in := gibs(gibIdx+1).io.ipinSW(j/2)
          val index = gibs(gibIdx+1).oPortMap("ipinSW" + (j/2).toString)
          connections.append((smi_id("GIB")(gibIdx+1), "GIB", index, smi_id("OB")(i), "OB", j))
        }
      }
    }      
  }

  // PE to GIB connections
  for(i <- 0 until rows){
    for(j <- 0 until cols){
      pes(i*cols+j).io.en := io.en(j)
      pes(i*cols+j).io.in.zipWithIndex.map{ case (in, k) =>
        if(k%4 == 0){ // northwest - southeast
          in := gibs(i*(cols+1)+j).io.ipinSE(k/4)
          val index = gibs(i*(cols+1)+j).oPortMap("ipinSE" + (k/4).toString)
          connections.append((smi_id("GIB")(i*(cols+1)+j), "GIB", index, smi_id("GPE")(i*cols+j), "GPE", k))
        } else if(k%4 == 1) { // northeast - southwest
          in := gibs(i*(cols+1)+j+1).io.ipinSW(k/4)
          val index = gibs(i*(cols+1)+j+1).oPortMap("ipinSW" + (k/4).toString)
          connections.append((smi_id("GIB")(i*(cols+1)+j+1), "GIB", index, smi_id("GPE")(i*cols+j), "GPE", k))
        } else if(k%4 == 2) { // southwest - northeast
          in := gibs((i+1)*(cols+1)+j).io.ipinNE(k/4)
          val index = gibs((i+1)*(cols+1)+j).oPortMap("ipinNE" + (k/4).toString)
          connections.append((smi_id("GIB")((i+1)*(cols+1)+j), "GIB", index, smi_id("GPE")(i*cols+j), "GPE", k))
        } else { // southeast - northwest
          in := gibs((i+1)*(cols+1)+j+1).io.ipinNW(k/4)
          val index = gibs((i+1)*(cols+1)+j+1).oPortMap("ipinNW" + (k/4).toString)
          connections.append((smi_id("GIB")((i+1)*(cols+1)+j+1), "GIB", index, smi_id("GPE")(i*cols+j), "GPE", k))
        }
      }
      pes(i*cols+j).io.out.zipWithIndex.map{ case (out, k) =>
        gibs(i*(cols+1)+j).io.opinSE(k) := out
        gibs(i*(cols+1)+j+1).io.opinSW(k) := out
        gibs((i+1)*(cols+1)+j).io.opinNE(k) := out
        gibs((i+1)*(cols+1)+j+1).io.opinNW(k) := out
        val index1 = gibs(i*(cols+1)+j).iPortMap("opinSE" + k.toString)
        val index2 = gibs(i*(cols+1)+j+1).iPortMap("opinSW" + k.toString)
        val index3 = gibs((i+1)*(cols+1)+j).iPortMap("opinNE" + k.toString)
        val index4 = gibs((i+1)*(cols+1)+j+1).iPortMap("opinNW" + k.toString)
        connections.append((smi_id("GPE")(i*cols+j), "GPE", k, smi_id("GIB")(i*(cols+1)+j), "GIB", index1))
        connections.append((smi_id("GPE")(i*cols+j), "GPE", k, smi_id("GIB")(i*(cols+1)+j+1), "GIB", index2))
        connections.append((smi_id("GPE")(i*cols+j), "GPE", k, smi_id("GIB")((i+1)*(cols+1)+j), "GIB", index3))
        connections.append((smi_id("GPE")(i*cols+j), "GPE", k, smi_id("GIB")((i+1)*(cols+1)+j+1), "GIB", index4))
      }
    }
  }

  // GIB to GIB connections
  if(numTrack > 0) {
    for (i <- 0 to rows) {
      for (j <- 0 to cols) {
        if (i == 0) {
          gibs(j).io.itrackN.map { in => in := 0.U }
          gibs(j).io.itrackS.zipWithIndex.map { case (in, k) =>
            in := gibs(cols + 1 + j).io.otrackN(k)
            val index1 = gibs(j).iPortMap("itrackS" + k.toString)
            val index2 = gibs(cols + 1 + j).oPortMap("otrackN" + k.toString)
            connections.append((smi_id("GIB")(cols + 1 + j), "GIB", index2, smi_id("GIB")(j), "GIB", index1))
          }
        } else if (i == rows) {
          gibs(i * (cols + 1) + j).io.itrackS.map { in => in := 0.U }
          gibs(i * (cols + 1) + j).io.itrackN.zipWithIndex.map { case (in, k) =>
            in := gibs((i - 1) * (cols + 1) + j).io.otrackS(k)
            val index1 = gibs(i * (cols + 1) + j).iPortMap("itrackN" + k.toString)
            val index2 = gibs((i - 1) * (cols + 1) + j).oPortMap("otrackS" + k.toString)
            connections.append((smi_id("GIB")((i - 1) * (cols + 1) + j), "GIB", index2, smi_id("GIB")(i * (cols + 1) + j), "GIB", index1))
          }
        } else {
          gibs(i * (cols + 1) + j).io.itrackN.zipWithIndex.map { case (in, k) =>
            in := gibs((i - 1) * (cols + 1) + j).io.otrackS(k)
            val index1 = gibs(i * (cols + 1) + j).iPortMap("itrackN" + k.toString)
            val index2 = gibs((i - 1) * (cols + 1) + j).oPortMap("otrackS" + k.toString)
            connections.append((smi_id("GIB")((i - 1) * (cols + 1) + j), "GIB", index2, smi_id("GIB")(i * (cols + 1) + j), "GIB", index1))
          }
          gibs(i * (cols + 1) + j).io.itrackS.zipWithIndex.map { case (in, k) =>
            in := gibs((i + 1) * (cols + 1) + j).io.otrackN(k)
            val index1 = gibs(i * (cols + 1) + j).iPortMap("itrackS" + k.toString)
            val index2 = gibs((i + 1) * (cols + 1) + j).oPortMap("otrackN" + k.toString)
            connections.append((smi_id("GIB")((i + 1) * (cols + 1) + j), "GIB", index2, smi_id("GIB")(i * (cols + 1) + j), "GIB", index1))
          }
        }
        if (j == 0) {
          gibs(i * (cols + 1) + j).io.itrackW.map { in => in := 0.U }
          gibs(i * (cols + 1) + j).io.itrackE.zipWithIndex.map { case (in, k) =>
            in := gibs(i * (cols + 1) + j + 1).io.otrackW(k)
            val index1 = gibs(i * (cols + 1) + j).iPortMap("itrackE" + k.toString)
            val index2 = gibs(i * (cols + 1) + j + 1).oPortMap("otrackW" + k.toString)
            connections.append((smi_id("GIB")(i * (cols + 1) + j + 1), "GIB", index2, smi_id("GIB")(i * (cols + 1) + j), "GIB", index1))
          }
        } else if (j == cols) {
          gibs(i * (cols + 1) + j).io.itrackE.map { in => in := 0.U }
          gibs(i * (cols + 1) + j).io.itrackW.zipWithIndex.map { case (in, k) =>
            in := gibs(i * (cols + 1) + j - 1).io.otrackE(k)
            val index1 = gibs(i * (cols + 1) + j).iPortMap("itrackW" + k.toString)
            val index2 = gibs(i * (cols + 1) + j - 1).oPortMap("otrackE" + k.toString)
            connections.append((smi_id("GIB")(i * (cols + 1) + j - 1), "GIB", index2, smi_id("GIB")(i * (cols + 1) + j), "GIB", index1))
          }
        } else {
          gibs(i * (cols + 1) + j).io.itrackW.zipWithIndex.map { case (in, k) =>
            in := gibs(i * (cols + 1) + j - 1).io.otrackE(k)
            val index1 = gibs(i * (cols + 1) + j).iPortMap("itrackW" + k.toString)
            val index2 = gibs(i * (cols + 1) + j - 1).oPortMap("otrackE" + k.toString)
            connections.append((smi_id("GIB")(i * (cols + 1) + j - 1), "GIB", index2, smi_id("GIB")(i * (cols + 1) + j), "GIB", index1))
          }
          gibs(i * (cols + 1) + j).io.itrackE.zipWithIndex.map { case (in, k) =>
            in := gibs(i * (cols + 1) + j + 1).io.otrackW(k)
            val index1 = gibs(i * (cols + 1) + j).iPortMap("itrackE" + k.toString)
            val index2 = gibs(i * (cols + 1) + j + 1).oPortMap("otrackW" + k.toString)
            connections.append((smi_id("GIB")(i * (cols + 1) + j + 1), "GIB", index2, smi_id("GIB")(i * (cols + 1) + j), "GIB", index1))
          }
        }
      }
    }
  }

  // apply("connections", connections)
  apply("connections", connections.zipWithIndex.map{case (c, i) => i -> c}.toMap)
  
  // Configurations, each row share one config bus
  val cfgRegNum = 2*rows+4
  val cfgRegs = RegInit(VecInit(Seq.fill(cfgRegNum)(0.U((1+cfgAddrWidth+cfgDataWidth).W))))
  cfgRegs(0) := Cat(io.cfg_en, io.cfg_addr, io.cfg_data)
  (1 until cfgRegNum).map{ i => cfgRegs(i) := cfgRegs(i-1) }
  for(j <- 0 until cols){ // top row
    ibs(j).io.cfg_en   := io.cfg_en
    ibs(j).io.cfg_addr := io.cfg_addr
    ibs(j).io.cfg_data := io.cfg_data
    obs(j).io.cfg_en   := cfgRegs(0)(cfgAddrWidth+cfgDataWidth)
    obs(j).io.cfg_addr := cfgRegs(0)(cfgAddrWidth+cfgDataWidth-1, cfgDataWidth)
    obs(j).io.cfg_data := cfgRegs(0)(cfgDataWidth-1, 0)
  }
  for(j <- 0 until cols){ // buttom row
    val idx = cols+j
    ibs(idx).io.cfg_en   := cfgRegs(cfgRegNum-2)(cfgAddrWidth+cfgDataWidth)
    ibs(idx).io.cfg_addr := cfgRegs(cfgRegNum-2)(cfgAddrWidth+cfgDataWidth-1, cfgDataWidth)
    ibs(idx).io.cfg_data := cfgRegs(cfgRegNum-2)(cfgDataWidth-1, 0)
    obs(idx).io.cfg_en   := cfgRegs(cfgRegNum-1)(cfgAddrWidth+cfgDataWidth)
    obs(idx).io.cfg_addr := cfgRegs(cfgRegNum-1)(cfgAddrWidth+cfgDataWidth-1, cfgDataWidth)
    obs(idx).io.cfg_data := cfgRegs(cfgRegNum-1)(cfgDataWidth-1, 0)
  }
  for(i <- 0 to rows){
    for(j <- 0 to cols){
      gibs(i*(cols+1)+j).io.cfg_en   := cfgRegs(2*i+1)(cfgAddrWidth+cfgDataWidth)
      gibs(i*(cols+1)+j).io.cfg_addr := cfgRegs(2*i+1)(cfgAddrWidth+cfgDataWidth-1, cfgDataWidth)
      gibs(i*(cols+1)+j).io.cfg_data := cfgRegs(2*i+1)(cfgDataWidth-1, 0)
      if((i < rows) && (j < cols)){
        pes(i*cols+j).io.cfg_en   := cfgRegs(2*i+2)(cfgAddrWidth+cfgDataWidth)
        pes(i*cols+j).io.cfg_addr := cfgRegs(2*i+2)(cfgAddrWidth+cfgDataWidth-1, cfgDataWidth)
        pes(i*cols+j).io.cfg_data := cfgRegs(2*i+2)(cfgDataWidth-1, 0)
      }
    }
  }      

  if(dumpIR){
    val outFilename = "src/main/resources/cgra_adg.json"
    printIR(outFilename)
  }

}





//object VerilogGen extends App {
//  val connect_flexibility = mutable.Map(
//    "num_itrack_per_ipin" -> 2, // ipin number = 3
//    "num_otrack_per_opin" -> 6, // opin number = 1
//    "num_ipin_per_opin"   -> 9
//  )
//  val attrs: mutable.Map[String, Any] = mutable.Map(
//    "num_row" -> 4,
//    "num_colum" -> 4,
//    "data_width" -> 32,
//    "cfg_data_width" -> 64,
//    "cfg_addr_width" -> 8,
//    "cfg_blk_offset" -> 2,
//    "num_rf_reg" -> 1,
//    "operations" -> ListBuffer("PASS", "ADD", "SUB", "MUL", "AND", "OR", "XOR", "SEL"),
//    "max_delay" -> 4,
//    "num_track" -> 3,
//    "connect_flexibility" -> connect_flexibility,
//    "num_output_ib" -> 3,
//    "num_input_ob" -> 6
//  )
//
//  (new chisel3.stage.ChiselStage).emitVerilog(new CGRA(attrs, true), args)
//}