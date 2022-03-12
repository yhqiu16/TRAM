
#include "mapper/visualize.h"


Graphviz::Graphviz(Mapping* mapping, std::string dirname) : _mapping(mapping), _dirname(dirname) {}


// create name for DFG node
std::string Graphviz::getDfgNodeName(int id, int idx, bool isDfgInput){
    std::string name;
    DFG* dfg = _mapping->getDFG();
    if(id != dfg->id()){
        name = dfg->node(id)->name();
    }else if(isDfgInput){
        name = dfg->inputName(idx);
    }else{
        name = dfg->outputName(idx);
    }
    return name;
}


void Graphviz::drawDFG(){
    std::string filename = _dirname + "/mapped_dfg.dot";
    std::ofstream ofs(filename);
    DFG* dfg = _mapping->getDFG();
    int dfgId = dfg->id();
    ofs << "Digraph G {\n";
    for(auto& elem : dfg->inputs()){
        std::string name = getDfgNodeName(dfgId, elem.first);
        std::string quoteName = "\"" + name + "\"";
        int lat = _mapping->dfgInputAttr(elem.first).lat;
        ofs << quoteName << "[label = \"\\N\\nlat=" << lat << "\"];\n";
    }
    for(auto& elem : dfg->outputs()){
        std::string name = getDfgNodeName(dfgId, elem.first, false);
        std::string quoteName = "\"" + name + "\"";
        int srcNodeId = elem.second.first;
        std::string srcName = getDfgNodeName(srcNodeId, elem.second.second);
        std::string quoteSrcName = "\"" + srcName + "\"";
        int lat = _mapping->dfgOutputAttr(elem.first).lat;
        ofs << quoteName << "[label = \"\\N\\nlat=" << lat << "\"];\n";
        ofs << quoteSrcName << "->" << quoteName << std::endl;
    }
    for(auto& elem : dfg->nodes()){
        auto node = elem.second;
        auto& attr = _mapping->dfgNodeAttr(node->id());
        auto name = getDfgNodeName(node->id());
        std::string quoteName = "\"" + name + "\"";
        ofs << quoteName << "[label = \"\\N\\nlat=" << attr.lat << "\"];\n";
        for(auto& input : node->inputs()){
            int srcNodeId = input.second.first;
            std::string srcName = getDfgNodeName(srcNodeId, input.second.second);
            std::string quoteSrcName = "\"" + srcName + "\"";
            ofs << quoteSrcName << "->" << quoteName << ";\n";
        }
    }
    ofs << "}\n";
}



// create name for ADG node
std::string Graphviz::getAdgNodeName(int id, int idx, bool isDfgInput){
    std::string name;
    ADG* adg = _mapping->getADG();
    if(id != adg->id()){
        name = adg->node(id)->type() + std::to_string(id);
    }else if(isDfgInput){
        name = "input" + std::to_string(idx);
    }else{
        name = "output" + std::to_string(idx);
    }
    return name;
}


void Graphviz::drawADG(){
    std::string filename = _dirname + "/mapped_adg.dot";
    std::ofstream ofs(filename);
    ADG* adg = _mapping->getADG();
    int adgId = adg->id();
    DFG* dfg = _mapping->getDFG();
    int dfgId = dfg->id();
    ofs << "Digraph G {\nlayout = sfdp;\noverlap = scale;\n";
    // assign DFG IO to ADG IO according to mapping result
    // _mapping->assignDfgIO();
    std::set<int> mappedInputs;
    for(auto& elem : dfg->inputs()){
        std::string dfgIOname = getDfgNodeName(dfgId, elem.first);
        int adgIOPort = _mapping->dfgInputAttr(elem.first).adgIOPort;
        mappedInputs.emplace(adgIOPort);
        auto adgIOname = getAdgNodeName(adgId, adgIOPort);
        ofs << adgIOname << "[label = \"" << adgIOname << "\\nDFG:" << dfgIOname << "\""
                << ", color = red];\n";
    }
    for(auto& elem : adg->inputs()){
        if(!mappedInputs.count(elem.first)){
            ofs << getAdgNodeName(adgId, elem.first) << ";\n";
        }        
    }
    std::set<int> mappedOutputs;
    for(auto& elem : dfg->outputs()){
        std::string dfgIOname = getDfgNodeName(dfgId, elem.first, false);
        int adgIOPort = _mapping->dfgOutputAttr(elem.first).adgIOPort;
        mappedOutputs.emplace(adgIOPort);
        auto adgIOname = getAdgNodeName(adgId, adgIOPort, false);
        ofs << adgIOname << "[label = \"" << adgIOname << "\\nDFG:" << dfgIOname << "\""
                << ", color = red];\n";
    }
    for(auto& elem : adg->outputs()){
        std::string name = getAdgNodeName(adgId, elem.first, false);
        int srcNodeId = elem.second.first;
        std::string srcName = getAdgNodeName(srcNodeId, elem.second.second);
        if(!mappedOutputs.count(elem.first)){
            ofs << name << ";\n";
        }        
        ofs << srcName << "->" << name << "[color = gray80];\n";
    }
    for(auto& elem : adg->nodes()){
        auto node = elem.second;
        auto& attr = _mapping->adgNodeAttr(node->id());
        auto name = getAdgNodeName(node->id());
        auto dfgNode = _mapping->mappedNode(node);
        if(dfgNode){
            ofs << name << "[label = \"" << name << "\\nDFG:" << getDfgNodeName(dfgNode->id()) << "\""
                << ", color = red];\n";
        }else{
            ofs << name << "[label = \"" << name << "\", color = black];\n";
        }
        std::set<int> srcNodeIds;
        for(auto& input : node->inputs()){
            int srcNodeId = input.second.first;
            if(srcNodeId == adgId){
                std::string srcName = getAdgNodeName(srcNodeId, input.second.second);
                ofs << srcName << "->" << name << "[color = gray80];\n"; 
            }else{ // only keep one connection
                srcNodeIds.emplace(srcNodeId);
            }           
            // int srcNodeId = input.second.first;
            // std::string srcName = getAdgNodeName(srcNodeId, input.second.second);
            // ofs << srcName << "->" << name << "[color = gray80];\n";        
        }
        for(auto srcNodeId : srcNodeIds){
            std::string srcName = getAdgNodeName(srcNodeId, 0);
            ofs << srcName << "->" << name << "[color = gray80];\n";  
        }
    }
    ofs << "edge [colorscheme=paired12];\n";
    for(auto& elem : dfg->edges()){
        int eid = elem.first;
        auto& attr = _mapping->dfgEdgeAttr(eid);
        auto& edgeLinks = attr.edgeLinks;
        int i = edgeLinks.size();
        for(auto& edgeLink : edgeLinks){
            i--;
            auto adgNode = edgeLink.adgNode;
            auto name = getAdgNodeName(adgNode->id());
            if(i > 0){
                if(adgNode->type() == "IB"){
                    int inport = adgNode->input(0).second; // IB has only one input port, connected to ADG Input port
                    auto IOname = getAdgNodeName(adgId, inport);
                    ofs << IOname << "->" << name << "->";
                }else{
                    ofs << name << "->";
                }               
            }else{
                if(adgNode->type() == "OB"){
                    int outport = adgNode->output(0).begin()->second; // OB has only one output port, connected to ADG output port
                    auto IOname = getAdgNodeName(adgId, outport, false);
                    ofs << name << "->" << IOname;
                }else{
                    ofs << name;
                }
                ofs << "[weight = 4, color = " << ((eid%12)+1) << "];\n";
            }
        }
        // // print edge path
        // auto dfgSrcNodeName = getDfgNodeName(elem.second->srcId(), elem.second->srcPortIdx(), true);
        // auto dfgDstNodeName = getDfgNodeName(elem.second->dstId(), elem.second->dstPortIdx(), false);
        // std::cout << dfgSrcNodeName << " => " << dfgDstNodeName << ": ";
        // i = edgeLinks.size();
        // for(auto& edgeLink : edgeLinks){
        //     auto adgNodeName = getAdgNodeName(edgeLink.adgNode->id());
        //     int adgNodeSrcPort = edgeLink.srcPort;
        //     int adgNodeDstPort = edgeLink.dstPort;
        //     std::cout << "(" << adgNodeName << ", " << adgNodeSrcPort << ", " << adgNodeDstPort;
        //     i--;
        //     if(i > 0){
        //         std::cout << ") -> ";
        //     }else{
        //         std::cout << ");\n";
        //     }
        // }
    }
    ofs << "}\n";
}


// print edge path
void Graphviz::printDFGEdgePath(){
    DFG* dfg = _mapping->getDFG();
    for(auto& elem : dfg->edges()){
        int eid = elem.first;
        auto& attr = _mapping->dfgEdgeAttr(eid);
        auto& edgeLinks = attr.edgeLinks;
        // print edge path
        auto dfgSrcNodeName = getDfgNodeName(elem.second->srcId(), elem.second->srcPortIdx(), true);
        auto dfgDstNodeName = getDfgNodeName(elem.second->dstId(), elem.second->dstPortIdx(), false);
        std::cout << dfgSrcNodeName << " => " << dfgDstNodeName << ":\n";
        int i = edgeLinks.size();
        for(auto& edgeLink : edgeLinks){
            auto adgNodeName = getAdgNodeName(edgeLink.adgNode->id());
            int adgNodeSrcPort = edgeLink.srcPort;
            int adgNodeDstPort = edgeLink.dstPort;
            std::cout << "(" << adgNodeName << ", " << adgNodeSrcPort << ", " << adgNodeDstPort;
            i--;
            if(i > 0){
                std::cout << ") -> ";
            }else{
                std::cout << ");\n";
            }
        }
    }
}


// dump mapped DFG IO ports with mapped ADG IO and latency annotated
void Graphviz::dumpDFGIO(){
    std::string filename = _dirname + "/mapped_dfgio.txt";
    std::ofstream ofs(filename);
    ADG* adg = _mapping->getADG();
    int adgId = adg->id();
    DFG* dfg = _mapping->getDFG();
    int dfgId = dfg->id();
    // if(assignIO){
    //     // assign DFG IO to ADG IO according to mapping result
    //     _mapping->assignDfgIO();
    // }
    ofs << "# format: DFG-IO-Name, ADG-IO-Index, DFG-IO-Latency\n";
    for(auto& elem : dfg->inputs()){
        std::string dfgIOname = getDfgNodeName(dfgId, elem.first);
        auto& attr =  _mapping->dfgInputAttr(elem.first);
        int adgIOPort = attr.adgIOPort;
        // auto adgIOname = getAdgNodeName(adgId, adgIOPort);
        ofs << dfgIOname << ", " << adgIOPort << ", " << attr.lat << std::endl;
    }
    for(auto& elem : dfg->outputs()){
        std::string dfgIOname = getDfgNodeName(dfgId, elem.first, false);
        auto& attr =  _mapping->dfgOutputAttr(elem.first);
        int adgIOPort = attr.adgIOPort;
        // auto adgIOname = getAdgNodeName(adgId, adgIOPort, false);
        ofs << dfgIOname << ", " << adgIOPort << ", " << attr.lat << std::endl;
    }
}