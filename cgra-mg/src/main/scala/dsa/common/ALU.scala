package dsa

import chisel3._
import chisel3.util._
import scala.collection.mutable.ListBuffer
import op._


/** reconfigurable arithmetic unit
 * 
 * @param width   data width
 */
class ALU(width: Int, ops: ListBuffer[OPC.OPC]) extends Module {
  val maxNumOperands = ops.map(OpInfo(width).getOperandNum(_)).max
  val cfgDataWidth = log2Ceil(OPC.numOPC)
  val io = IO(new Bundle {
    val config = Input(UInt(cfgDataWidth.W))
    val in = Input(Vec(maxNumOperands, UInt(width.W)))
    val out = Output(UInt(width.W)) 
  })

  val op2res = ops.map{op => 
    (op.id.U -> OpInfo.OpFuncMap(op)(io.in.toSeq))}

  io.out := MuxLookup(io.config, 0.U, op2res)

}


// object VerilogGen extends App {
//   (new chisel3.stage.ChiselStage).emitVerilog(new ALU(64, ListBuffer(OPC.ADD, OPC.SUB, OPC.SEL)),args)
// }
