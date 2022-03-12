package op
// ALU operations

import chisel3._
import chisel3.util._
import scala.collection.mutable
import scala.collection.mutable.ListBuffer
import ir._

/**
 * Operation Code
 */ 
object OPC extends Enumeration {
	type OPC = Value
	// Operation Code
	val PASS,    // passthrough from in to out
		ADD,     // add
	  SUB,     // substrate
	  MUL,     // multiply
	  // DIV,     // divide
	  // MOD,     // modulo
	  // MIN,
	  // NOT,
	  AND,
	  OR,
	  XOR,
	  SHL,     // shift left
	  LSHR,    // logic shift right
		ASHR,    // arithmetic shift right
//	  CSHL,    // cyclic shift left
//	  CSHR,    // cyclic shift right
	  EQ,      // equal to
	  NE,      // not equal to
	  LT,      // less than
	  LE,      // less than or equal to
//		SAT,		 // saturate value to a threshold
//		MGE,		 // merge two data
//		SPT,	   // split one data to two
	  SEL    = Value  // Select

	val numOPC = this.values.size

	def printOPC = {
		this.values.foreach{ op => println(s"$op\t: ${op.id}")}
	}
}

/** 
 *  Operation Information
 */ 
object OpInfo {
	val OpInfoMap: Map[OPC.OPC, List[Int]] = Map(
		// OPC -> List(NumOperands, NumRes, Latency, Operands-Commutative)
		// latency including the register outside ALU
		OPC.PASS -> List(1, 1, 1, 0),
		OPC.ADD  -> List(2, 1, 1, 1),
		OPC.SUB  -> List(2, 1, 1, 0),
		OPC.MUL  -> List(2, 1, 1, 1),
		// OPC.DIV  -> List(2, 1, 1, 0),
		// OPC.MOD  -> List(2, 1, 1, 0),
		// OPC.MIN  -> List(2, 1, 1, 1),
		OPC.AND  -> List(2, 1, 1, 1),
		OPC.OR   -> List(2, 1, 1, 1),
		OPC.XOR  -> List(2, 1, 1, 1),
		OPC.SHL  -> List(2, 1, 1, 0),
		OPC.LSHR -> List(2, 1, 1, 0),
		OPC.ASHR -> List(2, 1, 1, 0),
//		OPC.CSHL -> List(2, 1, 1, 0),
//		OPC.CSHR -> List(2, 1, 1, 0),
		OPC.EQ   -> List(2, 1, 1, 1),
		OPC.NE   -> List(2, 1, 1, 1),
		OPC.LT   -> List(2, 1, 1, 0),
		OPC.LE   -> List(2, 1, 1, 0),
//		OPC.SAT  -> List(2, 1, 1, 0),
		OPC.SEL  -> List(3, 1, 1, 0)
	)

	def getOperandNum(op: OPC.OPC): Int = {
		OpInfoMap(op)(0)
	}

	def getResNum(op: OPC.OPC): Int = {
		OpInfoMap(op)(1)
	}

	def getLatency(op: OPC.OPC): Int = {
		OpInfoMap(op)(2)
	}

	def isCommutative(op: OPC.OPC): Int = {
		OpInfoMap(op)(3)
	}

	def dumpOpInfo(filename: String): Unit = {
		val infos = ListBuffer[Map[String, Any]]();
		OPC.values.foreach{ op => 
			val info = Map(
				"name" -> op.toString(),
				"OPC" -> op.id,
				"numOperands" -> getOperandNum(op),
				"numRes" -> getResNum(op),
				"latency" -> getLatency(op),
				"commutative" -> isCommutative(op)
			)
			infos += info
		}
		val ops: mutable.Map[String,Any] = mutable.Map("Operations" -> infos)
		IRHandler.dumpIR(ops, filename)
	}

	private var width = 32
	private var high = width - 1

	def apply(opWidth: Int) = {
		width = opWidth
		high = width - 1
		this
	}

	val OpFuncMap: Map[OPC.OPC, Seq[UInt] => UInt] = Map(
		OPC.PASS -> ((ops: Seq[UInt]) => {
			ops(0)(high, 0) }),
		OPC.ADD  -> ((ops: Seq[UInt]) => {
			ops(0)(high, 0) + ops(1)(high, 0) }),
		OPC.SUB  -> ((ops: Seq[UInt]) => {
			ops(0)(high, 0) - ops(1)(high, 0) }),
		OPC.MUL  -> ((ops: Seq[UInt]) => {
			ops(0)(high, 0) * ops(1)(high, 0) }),
		// OPC.DIV  -> ((ops: Seq[UInt]) => {
		// 	ops(0)(high, 0) / ops(1)(high, 0) }),
		// OPC.MOD  -> ((ops: Seq[UInt]) => {
		// 	ops(0)(high, 0) % ops(1)(high, 0) }),
		// OPC.MIN  -> ((ops: Seq[UInt]) => {
		// 	val op0 = ops(0)(high, 0) 
		// 	val op1 = ops(1)(high, 0) 
		// 	Mux(op0 < op1, op0, op1)}),
		OPC.AND  -> ((ops: Seq[UInt]) => {
			ops(0)(high, 0) & ops(1)(high, 0) }),
		OPC.OR   -> ((ops: Seq[UInt]) => {
			ops(0)(high, 0) | ops(1)(high, 0) }),
		OPC.XOR  -> ((ops: Seq[UInt]) => {
			ops(0)(high, 0) ^ ops(1)(high, 0) }),
		OPC.SHL  -> ((ops: Seq[UInt]) => {
			val shn = ops(1)(log2Ceil(width)-1, 0) // shift number
			ops(0)(high, 0) << shn }),
		OPC.LSHR  -> ((ops: Seq[UInt]) => {
			val shn = ops(1)(log2Ceil(width)-1, 0) // shift number
			ops(0)(high, 0) >> shn }),
		OPC.ASHR  -> ((ops: Seq[UInt]) => {
			val shn = ops(1)(log2Ceil(width)-1, 0) // shift number
			(ops(0)(high, 0).asSInt >> shn).asUInt }),
//		OPC.CSHL -> ((ops: Seq[UInt]) => {
//			val shn = ops(1)(log2Ceil(width)-1, 0) // shift number
//			(ops(0)(high, 0) << shn) | (ops(0)(high, 0) >> (width.U - shn)) }),
//		OPC.CSHR -> ((ops: Seq[UInt]) => {
//			val shn = ops(1)(log2Ceil(width)-1, 0) // shift number
//			(ops(0)(high, 0) >> shn) | (ops(0)(high, 0) << (width.U - shn)) }),
		OPC.EQ   -> ((ops: Seq[UInt]) => {
			ops(0)(high, 0) === ops(1)(high, 0) }),
		OPC.NE   -> ((ops: Seq[UInt]) => {
			ops(0)(high, 0) =/= ops(1)(high, 0) }),
		OPC.LT   -> ((ops: Seq[UInt]) => {
			ops(0)(high, 0) < ops(1)(high, 0) }),
		OPC.LE   -> ((ops: Seq[UInt]) => {
			ops(0)(high, 0) <= ops(1)(high, 0) }),
//		OPC.SAT  -> ((ops: Seq[UInt]) => {
//			Mux(ops(0)(high, 0) <= ops(1)(high, 0), ops(0)(high, 0), ops(1)(high, 0)) }),
		OPC.SEL  -> ((ops: Seq[UInt]) => {
			Mux(ops(2)(0), ops(1)(high, 0), ops(0)(high, 0)) })
	)
}


//object test extends App {
//	println(OPC.values)
//	println(s"OPC number: ${OPC.numOPC}")
//	println(OPC(0), OPC.withName("ADD"))
//	OPC.printOPC
//	val outFilename = "src/main/resources/operations.json"
//	OpInfo.dumpOpInfo(outFilename)
//	// println(OpInfo.OpFuncMap(OPC.ADD)(Seq(1.U, 2.U)))
//}