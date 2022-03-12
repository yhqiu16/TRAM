package dsa

import chisel3._
import chisel3.util._


/** Single port SRAM
 * 
 * @param width   data I/O width
 * @param depth   SRAM depth
 */ 
class SinglePortSRAM(width: Int, depth: Int) extends Module {
  val io = IO(new Bundle{
    val en = Input(Bool())
    val we = Input(Bool())
    val addr = Input(UInt(log2Ceil(depth).W))
    val din  = Input(UInt(width.W))
    val dout = Output(UInt(width.W)) // Latency = 1
  })

  val mem = Mem(depth, UInt(width.W))
  val dout = RegInit(0.U(width.W))

    io.dout := dout
  
    when(io.en) {
      when(io.we) {
        mem(io.addr) := io.din
      }.otherwise {
        dout := mem(io.addr) 
      }
    }
}



/** Simple dual port SRAM
 * 
 * @param width   data I/O width
 * @param depth   SRAM depth
 */ 
class SimpleDualPortSRAM(width: Int, depth: Int) extends Module {
  val io = IO(new Bundle{
    val ena   = Input(Bool())
    val wea   = Input(Bool())
    val addra = Input(UInt(log2Ceil(depth).W))
    val dina  = Input(UInt(width.W))
    val enb   = Input(Bool())
    val addrb = Input(UInt(log2Ceil(depth).W))
    val doutb = Output(UInt(width.W)) // Latency = 1
  })

  val mem = Mem(depth, UInt(width.W))
  val doutb = RegInit(0.U(width.W))

    io.doutb := doutb
  
    when(io.ena && io.wea) {
      mem(io.addra) := io.dina
    }
    when(io.enb) {
      doutb := mem(io.addrb) 
    }
}



/** True dual port SRAM
 * 
 * @param width   data I/O width
 * @param depth   SRAM depth
 */ 
class TrueDualPortSRAM(width: Int, depth: Int) extends Module {
  val io = IO(new Bundle{
    val ena   = Input(Bool())
    val wea   = Input(Bool())
    val addra = Input(UInt(log2Ceil(depth).W))
    val dina  = Input(UInt(width.W))
    val douta = Output(UInt(width.W)) // Latency = 1
    val enb   = Input(Bool())
    val web   = Input(Bool())
    val addrb = Input(UInt(log2Ceil(depth).W))
    val dinb  = Input(UInt(width.W))
    val doutb = Output(UInt(width.W)) // Latency = 1
  })

  val mem = Mem(depth, UInt(width.W))
  val douta = RegInit(0.U(width.W))
  val doutb = RegInit(0.U(width.W))

    io.douta := douta
    io.doutb := doutb
    
    when(io.ena) {
      when(io.wea) {
        mem(io.addra) := io.dina
      }.otherwise {
        douta := mem(io.addra) 
      }
    }

    when(io.enb) {
      when(io.web) {
        mem(io.addrb) := io.dinb
      }.otherwise {
        doutb := mem(io.addrb) 
      }
    }
}



// object VerilogGen extends App {
//   (new chisel3.stage.ChiselStage).emitVerilog(new LUT(2, 4),args)
// }
