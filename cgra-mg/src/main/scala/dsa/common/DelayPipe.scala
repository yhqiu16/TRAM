package dsa

import chisel3._
import chisel3.util._
import scala.math.pow


/** Delay-configurable Pipe
 * 
 * @param width     data width
 * @param maxDelay  max delay cycles
 */
class DelayPipe(width: Int, maxDelay: Int) extends Module {
  val cfgWidth = log2Ceil(maxDelay+1)
  val io = IO(new Bundle {
    val en = Input(Bool())
    val config = Input(UInt(cfgWidth.W)) // delay cycles
    val in = Input(UInt(width.W))
    val out = Output(UInt(width.W))
  })

  val regs = RegInit(VecInit(Seq.fill(maxDelay+1){0.U(width.W)}))
  val wptr = RegInit(0.U(cfgWidth.W))   // write pointer
  val rptr = RegInit(0.U(cfgWidth.W))   // read pointer
  
  when(io.en && (wptr < maxDelay.U)){
    wptr := wptr+1.U
  }.otherwise{
    wptr := 0.U
  }

  when(wptr+1.U >= io.config){
    rptr := wptr + 1.U - io.config
  }.otherwise{
    rptr := (2 + maxDelay).U + wptr - io.config
  }


  when(io.en && (io.config > 0.U)){
    regs(wptr) := io.in
  }

  val cnt = RegInit(0.U(cfgWidth.W)) // counter
  when(!io.en){
    cnt := 0.U
  }.elsewhen(cnt < io.config){
    cnt := cnt + 1.U
  }

  when(io.en && (0.U === io.config)){
    io.out := io.in  // delay = 0
  }.elsewhen(io.en && (cnt === io.config)){
    io.out := regs(rptr)
  }.otherwise{ // out 0 before the data is written into the position 
    io.out := 0.U
  }

}



// object VerilogGen extends App {
//   (new chisel3.stage.ChiselStage).emitVerilog(new DelayPipe(32, 7),args)
// }

