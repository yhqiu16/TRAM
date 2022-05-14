package real

import dsl._

object softbrain extends App {
  // Keys that differ nodes
  identifier("row_idx","col_idx")

  // Define Default Switch
  val sw_default = new ssnode("switch")

  // Define General Function Unit
  val fu_general = new ssnode("processing element")
  fu_general(
    "instructions", List("Add", "Sub", "Mul", "And32", "Or32", "Xor32") )( // "LShf", "RShf", "EQ", "NQ", "LT", "LE"
    "max_delay", 4)(
    "register_file_size", 1
    )

  // sw_default <-> fu_general

  // Define Softbrain
  val softbrain = new ssfabric
  softbrain.apply("name", "softbrain")
  softbrain(
    "default_data_width", 32)(
    "default_flow_control", false)(
    "default_granularity", 32)(
    "default_max_util", 1)

  // Build Topology
  val numRow = 5
  val numCol = 5
  val switchMesh = softbrain.buildMesh(sw_default, numRow, numCol)

  // Add the function unit
  for(row_idx <- 0 until numRow-1;col_idx <- 0 until numCol-1){
    val temp_node = fu_general.clone
    temp_node("row_idx",row_idx)("col_idx",col_idx)
    softbrain(
      softbrain(row_idx)(col_idx)("switch") --> temp_node
    )(
      softbrain(row_idx+1)(col_idx)("switch") --> temp_node
    )(
      softbrain(row_idx)(col_idx+1)("switch") --> temp_node
    )(
      softbrain(row_idx+1)(col_idx+1)("switch") <-> temp_node
    )
  }

  // Connect the Vector port
  for(col_idx <- 0 until numCol-1){
    // val in_vport = new ssnode("vector port")
    // softbrain(
    //   in_vport --> softbrain(0)(col_idx)("switch")     
    // )(
    //   in_vport --> softbrain(0)(col_idx+1)("switch")
    // )
    // val out_vport = new ssnode("vector port")
    // softbrain(
    //   softbrain(numRow-1)(col_idx)("switch") --> out_vport
    // )(
    //   softbrain(numRow-1)(col_idx+1)("switch") --> out_vport
    // )
    val in_vport1 = new ssnode("vector port")
    softbrain(
      in_vport1 --> softbrain(0)(col_idx)("switch")     
    )
    val in_vport2 = new ssnode("vector port")
    softbrain(
      in_vport2 --> softbrain(0)(col_idx+1)("switch")
    )
    val out_vport1 = new ssnode("vector port")
    softbrain(
      softbrain(numRow-1)(col_idx)("switch") --> out_vport1
    )
    val out_vport2 = new ssnode("vector port")
    softbrain(
      softbrain(numRow-1)(col_idx+1)("switch") --> out_vport2
    )
    // val in_vport3 = new ssnode("vector port")
    // softbrain(
    //   in_vport3 --> softbrain(0)(col_idx)("switch")     
    // )
    // val in_vport4 = new ssnode("vector port")
    // softbrain(
    //   in_vport4 --> softbrain(0)(col_idx+1)("switch")
    // )
    // val out_vport3 = new ssnode("vector port")
    // softbrain(
    //   softbrain(numRow-1)(col_idx)("switch") --> out_vport3
    // )
    // val out_vport4 = new ssnode("vector port")
    // softbrain(
    //   softbrain(numRow-1)(col_idx+1)("switch") --> out_vport4
    // )
  }
  // val in_vport1 = new ssnode("vector port")
  // val in_vport2 = new ssnode("vector port")
  // val out_vport = new ssnode("vector port")

  // // Connect IO
  // softbrain(in_vport1 |=> softbrain.filter("row_idx","nodeType")(0,"switch"))
  // softbrain(in_vport2 |=> softbrain.filter("row_idx","nodeType")(0,"switch"))
  // // softbrain(in_vport2 |=> softbrain.filter("col_idx","nodeType")(0,"switch"))
  // softbrain(out_vport <=| softbrain.filter("row_idx","nodeType")(numRow-1,"switch"))

  // in_vport1 |=> List(sw_default, sw_default, sw_default)

  // Print
  softbrain.printIR
}
