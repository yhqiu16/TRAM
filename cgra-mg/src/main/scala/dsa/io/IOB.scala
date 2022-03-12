package dsa

import chisel3._
import chisel3.util._
import scala.collection.mutable
import scala.collection.mutable.ListBuffer
import ir._


/** IO Block
 * 
 * @param attrs     module attributes
 */ 
class IOB(attrs: mutable.Map[String, Any]) extends Module with IR {
  apply(attrs)
  val width = getAttrValue("data_width").asInstanceOf[Int]
  val numIn = getAttrValue("num_input").asInstanceOf[Int]
  val numOut = getAttrValue("num_output").asInstanceOf[Int]
  // cfgParams
  val cfgDataWidth = getAttrValue("cfg_data_width").asInstanceOf[Int]
  val cfgAddrWidth = getAttrValue("cfg_addr_width").asInstanceOf[Int] 
  val cfgBlkIndex  = getAttrValue("cfg_blk_index").asInstanceOf[Int]     // configuration index of this block, cfg_addr[width-1 : offset] 
  val cfgBlkOffset = getAttrValue("cfg_blk_offset").asInstanceOf[Int]   // configuration offset bit of blocks
  
  val io = IO(new Bundle {
    val cfg_en   = Input(Bool())
    val cfg_addr = Input(UInt(cfgAddrWidth.W))
    val cfg_data = Input(UInt(cfgDataWidth.W))
    val in = Input(Vec(numIn, UInt(width.W)))
    val out = Output(Vec(numOut, UInt(width.W)))
  })

  val selWidth = log2Ceil(numIn) // width of the register selecting signal
  val sumCfgWidth = numOut*selWidth
  if(numIn > 1){
    val muxs = Array.fill(numOut){Module(new Muxn(width, numIn)).io}    
    val cfg = Module(new ConfigMem(sumCfgWidth, 1, cfgDataWidth)) // configuration memory
    cfg.io.cfg_en := io.cfg_en && (cfgBlkIndex.U === io.cfg_addr(cfgAddrWidth-1, cfgBlkOffset))
    cfg.io.cfg_addr := io.cfg_addr(cfgBlkOffset-1, 0)
    cfg.io.cfg_data := io.cfg_data
    assert(cfg.cfgAddrWidth <= cfgBlkOffset)
    assert(cfgBlkIndex < (1 << (cfgAddrWidth-cfgBlkOffset)))

    for(i <- 0 until numOut){
      for(j <- 0 until numIn){
        muxs(i).in(j) := io.in(j)
      }
      io.out(i) := muxs(i).out
      muxs(i).config := cfg.io.out(0)((i+1)*selWidth-1, i*selWidth)
    }

  } else{ // numIn == 1
    (0 until numOut).map{ i => io.out(i) := io.in(0) }
  }
  

  // ======= sub_module attribute ========//
  if(numIn > 1){
    val sub_modules = (0 until 1).map{ i => Map(
      "id" -> 1, 
      "type" -> "Muxn"
    )}
    apply("sub_modules", sub_modules)
  }

  // ======= sub_module instance attribute ========//
  // apply("sub_module_format", ("id", "type"))
  // 0 : this module
  // 1-n : sub-modules 
  val smi_id: Map[String, List[Int]] = {
    if(numIn > 1){
      Map(
        "This" -> List(0),
        "Muxn" -> (1 until numOut+1).toList
      )
    } else{
      Map("This" -> List(0))
    }
  }
  val instances = smi_id.map{case (name, ids) =>
    ids.map{id => Map(
      "id" -> id, 
      "type" -> name,
      "module_id" -> {if(name == "This") 0 else 1}
    )}
  }.flatten
  apply("instances", instances)


  // ======= connections attribute ========//
  // apply("connection_format", ("src_id", "src_type", "src_out_idx", "dst_id", "dst_type", "dst_in_idx"))
  // This:src_out_idx is the input index
  // This:dst_in_idx is the output index
  val connections = ListBuffer[(Int, String, Int, Int, String, Int)]()

  if(numIn > 1){
    for(i <- 0 until numOut){
      for(j <- 0 until numIn){
        connections.append((smi_id("This")(0), "This", j, smi_id("Muxn")(i), "Muxn", j))
      }
      connections.append((smi_id("Muxn")(i), "Muxn", 0, smi_id("This")(0), "This", i))
    }
  } else{ // numIn == 1
    (0 until numOut).map{ i => 
      connections.append((smi_id("This")(0), "This", 0, smi_id("This")(0), "This", i))
    }
  }
  // apply("connections", connections)
  apply("connections", connections.zipWithIndex.map{case (c, i) => i -> c}.toMap)


  // ======= configuration attribute ========//
  if(numIn > 1){
    val configuration = mutable.Map( // id : type, high, low
      smi_id("This")(0) -> ("This", sumCfgWidth-1, 0)
    )
    for(i <- 0 until numOut){
      configuration += smi_id("Muxn")(i) -> ("Muxn", (i+1)*selWidth-1, i*selWidth)
    }
    apply("configuration", configuration)
  }
  

}


