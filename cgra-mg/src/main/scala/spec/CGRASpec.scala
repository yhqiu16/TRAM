 package spec
 // Architecture Specification

 import scala.collection.mutable
 import scala.collection.mutable.ListBuffer
 import ir._


 // CGRA Specification
 object CGRASpec{
   val connect_flexibility = mutable.Map(
     "num_itrack_per_ipin" -> 2, // ipin number = 3
     "num_otrack_per_opin" -> 6, // opin number = 1
     "num_ipin_per_opin"   -> 6
   )
   val attrs: mutable.Map[String, Any] = mutable.Map(
     "num_row" -> 4,
     "num_colum" -> 4,
     "data_width" -> 32,
     "cfg_data_width" -> 64,
     "cfg_addr_width" -> 8,
     "cfg_blk_offset" -> 2,
     "num_rf_reg" -> 1,
     "operations" -> ListBuffer("PASS", "ADD", "SUB", "MUL", "AND", "OR", "XOR", "SEL"),
     "max_delay" -> 4,
     "num_track" -> 3,
     "track_reged_mode" -> 1,
     "connect_flexibility" -> connect_flexibility,
     "diag_iopin_connect" -> true,
     "num_output_ib" -> 3,
     "num_input_ob" -> 6
   )

   def loadSpec(jsonFile : String): Unit ={
     val jsonMap = IRHandler.loadIR(jsonFile)
     for(kv <- jsonMap){
       if(attrs.contains(kv._1)){
         if(kv._1 == "operations") {
           attrs(kv._1) = kv._2.asInstanceOf[List[String]].to[ListBuffer]
         }else if(kv._1 == "connect_flexibility"){
           attrs(kv._1) = mutable.Map() ++ kv._2.asInstanceOf[Map[String, Int]]
         }else{
           attrs(kv._1) = kv._2
         }
       }
     }
   }

   def dumpSpec(jsonFile : String): Unit={
     IRHandler.dumpIR(attrs, jsonFile)
   }

 }


