package dsa

import chisel3._
import chisel3.util._
import scala.collection.mutable
import scala.collection.mutable.{ListBuffer, ArrayBuffer}
import scala.math.{pow}
import op._
import ir._


/** GPE: Generic Processing Element
 * 
 * @param attrs     module attributes
 */ 
class GPE(attrs: mutable.Map[String, Any]) extends Module with IR {
  apply(attrs)
  val width = getAttrValue("data_width").asInstanceOf[Int]
  // val numIn = getAttrValue("num_input").asInstanceOf[Int]
  // val numOut = getAttrValue("num_output").asInstanceOf[Int]
  // cfgParams
  val cfgDataWidth = getAttrValue("cfg_data_width").asInstanceOf[Int]
  val cfgAddrWidth = getAttrValue("cfg_addr_width").asInstanceOf[Int] 
  val cfgBlkIndex  = getAttrValue("cfg_blk_index").asInstanceOf[Int]     // configuration index of this block, cfg_addr[width-1 : offset] 
  val cfgBlkOffset = getAttrValue("cfg_blk_offset").asInstanceOf[Int]   // configuration offset bit of blocks
  // number of registers in Regfile
  val numRegRF = getAttrValue("num_rf_reg").asInstanceOf[Int]
  // supported operations
  val opsStr = getAttrValue("operations").asInstanceOf[ListBuffer[String]]
  val ops = opsStr.map(OPC.withName(_))
  val aluOperandNum = ops.map(OpInfo.getOperandNum(_)).max
  // val aluCfgWidth = log2Ceil(OPC.numOPC) // ALU Config width
  val numInPerOperand = getAttrValue("num_input_per_operand").asInstanceOf[ListBuffer[Int]]
  // max delay cycles of the DelayPipe
  val maxDelay = getAttrValue("max_delay").asInstanceOf[Int]
  apply("num_operands", aluOperandNum)
  apply("num_input", numInPerOperand.sum)
  apply("num_output", 1)


  val io = IO(new Bundle {
    val cfg_en = Input(Bool())
    val cfg_addr = Input(UInt(cfgAddrWidth.W))
    val cfg_data = Input(UInt(cfgDataWidth.W))
    val en = Input(Bool())
    val in = Input(Vec(numInPerOperand.sum, UInt(width.W)))   
    val out = Output(Vec(1, UInt(width.W))) 
  })

  val alu = Module(new ALU(width, ops))
  val rf = Module(new RF(width, numRegRF, 1, 2))
  val delay_pipes = Array.fill(aluOperandNum){ Module(new DelayPipe(width, maxDelay)).io } 

  val const = Wire(UInt(width.W))
  val imuxs = numInPerOperand.map{ num => Module(new Muxn(width, num+2)).io } // input + const + rf_out 

  var offset = 0
  for(i <- 0 until aluOperandNum){
    val num = numInPerOperand(i)
    imuxs(i).in.zipWithIndex.map{ case (in, j) => 
      if(j < num) {
        in := io.in(offset+j) 
      } else if(j == num) {
        in := const
      } else {
        in := rf.io.out(1)
      }
    }
    delay_pipes(i).en := io.en
    delay_pipes(i).in := imuxs(i).out
    alu.io.in(i) := delay_pipes(i).out
    offset += num
  }

  rf.io.en := io.en
  rf.io.in(0) := alu.io.out
  io.out(0) := rf.io.out(0)

  // configuration memory
  val constCfgWidth = width // constant 
  val aluCfgWidth = alu.io.config.getWidth // ALU Config width
  val rfCfgWidth = rf.io.config.getWidth  // RF
  val delayCfgWidthEach = delay_pipes(0).config.getWidth // DelayPipe Config width
  val delayCfgWidth = aluOperandNum * delayCfgWidthEach
  val imuxCfgWidthList = imuxs.map{ mux => mux.config.getWidth } // input Muxes
  val imuxCfgWidth = imuxCfgWidthList.sum 
  val sumCfgWidth = constCfgWidth + aluCfgWidth + rfCfgWidth + delayCfgWidth + imuxCfgWidth

  val cfg = Module(new ConfigMem(sumCfgWidth, 1, cfgDataWidth))
  cfg.io.cfg_en := io.cfg_en && (cfgBlkIndex.U === io.cfg_addr(cfgAddrWidth-1, cfgBlkOffset))
  cfg.io.cfg_addr := io.cfg_addr(cfgBlkOffset-1, 0)
  cfg.io.cfg_data := io.cfg_data
  assert(cfg.cfgAddrWidth <= cfgBlkOffset)
  assert(cfgBlkIndex < (1 << (cfgAddrWidth-cfgBlkOffset)))

  val cfgOut = Wire(UInt(sumCfgWidth.W))
  cfgOut := cfg.io.out(0)
  const := cfgOut(constCfgWidth-1, 0)
  if(aluCfgWidth != 0){
    alu.io.config := cfgOut(constCfgWidth+aluCfgWidth-1, constCfgWidth)
  } else {
    alu.io.config := DontCare
  }
  if(rfCfgWidth != 0){
    rf.io.config := cfgOut(constCfgWidth+aluCfgWidth+rfCfgWidth-1, constCfgWidth+aluCfgWidth)
  } else{
    rf.io.config := DontCare
  }
  
  offset = constCfgWidth+aluCfgWidth+rfCfgWidth
  for(i <- 0 until aluOperandNum){
    if(delayCfgWidthEach != 0){
      delay_pipes(i).config := cfgOut(offset+delayCfgWidthEach-1, offset)
    } else {
      delay_pipes(i).config := DontCare
    }  
    offset += delayCfgWidthEach
  }
  for(i <- 0 until aluOperandNum){
    if(imuxCfgWidthList(i) != 0){
      imuxs(i).config := cfgOut(offset+imuxCfgWidthList(i)-1, offset)
    } else {
      imuxs(i).config := DontCare
    }    
    offset += imuxCfgWidthList(i)
  }


  // ======= sub_module attribute ========//
  // 1 : Constant
  // 2-n : sub-modules 
  val sm_id: Map[String, Int] = Map(
    "Const" -> 1,
    "ALU" -> 2,
    "RF" -> 3,
    "DelayPipe" -> 4,
    "Muxn" -> 5
  )

  val sub_modules = sm_id.map{case (name, id) => Map(
    "id" -> id, 
    "type" -> name
  )}
  apply("sub_modules", sub_modules)

  // ======= sub_module instance attribute ========//
  // 0 : this module
  // 1 : Constant
  // 2-n : sub-modules 
  val smi_id: Map[String, List[Int]] = Map(
    "This" -> List(0),
    "Const" -> List(1),
    "ALU" -> List(2),
    "RF" -> List(3),
    "DelayPipe" -> (4 until aluOperandNum+4).toList,
    "Muxn" -> (aluOperandNum+4 until 2*aluOperandNum+4).toList
  )
  val instances = smi_id.map{case (name, ids) =>
    ids.map{id => Map(
      "id" -> id, 
      "type" -> name,
      "module_id" -> {if(name == "This") 0 else sm_id(name)}
    )}
  }.flatten
  apply("instances", instances)

  // ======= connections attribute ========//
  // apply("connection_format", ("src_id", "src_type", "src_out_idx", "dst_id", "dst_type", "dst_in_idx"))
  // This:src_out_idx is the input index
  // This:dst_in_idx is the output index
  val connections = ListBuffer(
    (smi_id("ALU")(0), "ALU", 0, smi_id("RF")(0), "RF", 0), 
    (smi_id("RF")(0), "RF", 0, smi_id("This")(0), "This", 0)
  )

  offset = 0
  for(i <- 0 until aluOperandNum){
    val num = numInPerOperand(i)
    for(j <- 0 until num+2){
      if(j < num) {
        connections.append((smi_id("This")(0), "This", offset+j, smi_id("Muxn")(i), "Muxn", j)) 
      } else if(j == num) {
        connections.append((smi_id("Const")(0), "Const", 0, smi_id("Muxn")(i), "Muxn", j)) 
      } else {
        connections.append((smi_id("RF")(0), "RF", 1, smi_id("Muxn")(i), "Muxn", j))
      }
    }
    connections.append((smi_id("Muxn")(i), "Muxn", 0, smi_id("DelayPipe")(i), "DelayPipe", 0))
    connections.append((smi_id("DelayPipe")(i), "DelayPipe", 0, smi_id("ALU")(0), "ALU", i))
    offset += num
  }
  // apply("connections", connections)
  apply("connections", connections.zipWithIndex.map{case (c, i) => i -> c}.toMap)

  // ======= configuration attribute ========//
  val configuration = mutable.Map( // id : type, high, low
    smi_id("This")(0) -> ("This", sumCfgWidth-1, 0),
    smi_id("Const")(0) -> ("Const", constCfgWidth-1, 0)
  )
  if(aluCfgWidth != 0){
    configuration += smi_id("ALU")(0) -> ("ALU", constCfgWidth+aluCfgWidth-1, constCfgWidth)
  } 
  if(rfCfgWidth != 0){
    configuration += smi_id("RF")(0) -> ("ALU", constCfgWidth+aluCfgWidth+rfCfgWidth-1, constCfgWidth+aluCfgWidth)
  } 
  offset = constCfgWidth+aluCfgWidth+rfCfgWidth
  for(i <- 0 until aluOperandNum){
    if(delayCfgWidthEach != 0){
      configuration += smi_id("DelayPipe")(i) -> ("DelayPipe", offset+delayCfgWidthEach-1, offset)
    } 
    offset += delayCfgWidthEach
  }
  for(i <- 0 until aluOperandNum){
    if(imuxCfgWidthList(i) != 0){
      configuration += smi_id("Muxn")(i) -> ("Muxn", offset+imuxCfgWidthList(i)-1, offset)
    } 
    offset += imuxCfgWidthList(i)
  }
  apply("configuration", configuration)

  // val outFilename = "test_run_dir/my_cgra_test.json"
  // printIR(outFilename)
}






// object VerilogGen extends App {
//   val attrs: mutable.Map[String, Any] = mutable.Map(
//     "data_width" -> 32,
//     "cfg_data_width" -> 128,
//     "cfg_addr_width" -> 8,
//     "cfg_blk_index" -> 1,
//     "cfg_blk_offset" -> 4,
//     "num_rf_reg" -> 1,
//     "operations" -> ListBuffer("ADD", "SUB", "SEL"),
//     "num_input_per_operand" -> ListBuffer(4, 4, 4),
//     "max_delay" -> 7
//   )

//   (new chisel3.stage.ChiselStage).emitVerilog(new GPE(attrs),args)
// }