
#include "ir/adg_ir.h"


ADGIR::ADGIR(std::string filename)
{
    std::ifstream ifs(filename);
    if(!ifs){
        std::cout << "Cannnot open ADG file: " << filename << std::endl;
        exit(1);
    }
    json adgJson;
    ifs >> adgJson;
    _adg = parseADG(adgJson);
}

ADGIR::~ADGIR()
{
    if(_adg){
        delete _adg;
    }
}


// parse ADG json object
ADG* ADGIR::parseADG(json& adgJson){
    // std::cout << "Parse ADG..." << std::endl;
    ADG* adg = new ADG();
    adg->setBitWidth(adgJson["data_width"].get<int>());
    adg->setNumInputs(adgJson["num_input"].get<int>());
    adg->setNumOutputs(adgJson["num_output"].get<int>());  
    adg->setCfgDataWidth(adgJson["cfg_data_width"].get<int>());
    adg->setCfgAddrWidth(adgJson["cfg_addr_width"].get<int>());
    adg->setCfgBlkOffset(adgJson["cfg_blk_offset"].get<int>());
    std::map<int, std::pair<ADGNode*, bool>> modules; // // <moduleId, <ADGNode*, used>>
    for(auto& nodeJson : adgJson["sub_modules"]){
        ADGNode* node = parseADGNode(nodeJson);
        modules[node->id()] = std::make_pair(node, false);
    }
    for(auto& nodeJson : adgJson["instances"]){
        ADGNode* node = parseADGNode(nodeJson, modules);
        int nodeId = nodeJson["id"].get<int>();
        if(node){ // not store sub-module of "This" type
            adg->addNode(nodeId, node);
        }else{ // "This" sub-module
            adg->setId(nodeId);
        }
    }
    parseADGLinks(adg, adgJson["connections"]);
    postProcess(adg);  
    return adg; 
}


// parse ADGNode from sub-modules json object 
ADGNode* ADGIR::parseADGNode(json& nodeJson){
    // std::cout << "Parse ADG node" << std::endl;
    std::string type = nodeJson["type"].get<std::string>();
    // if(type == "This"){
    //     return nullptr;
    // }
    int nodeId = nodeJson["id"].get<int>();
    ADGNode* adg_node;
    if(type == "GPE" || type == "GIB" || type == "IB" || type == "OB"){
        auto& attrs = nodeJson["attributes"];
        int bitWidth = attrs["data_width"].get<int>();
        int numInputs = attrs["num_input"].get<int>();
        int numOutputs = attrs["num_output"].get<int>();
        int cfgBlkIdx = attrs["cfg_blk_index"].get<int>();
        // std::cout << "Parse sub-ADG..." << std::endl;
        ADG* subADG = parseADG(attrs); // parse sub-adg
        if(type == "GPE"){
            GPENode* node = new GPENode(nodeId);
            node->setNumRfReg(attrs["num_rf_reg"].get<int>());
            node->setMaxDelay(attrs["max_delay"].get<int>());
            node->setNumOperands(attrs["num_operands"].get<int>());
            for(auto& op : attrs["operations"]){
                node->addOperation(op.get<std::string>());
            }
            adg_node = node;
        }else if(type == "GIB"){
            GIBNode* node  = new GIBNode(nodeId);
            adg_node = node;
        }else if(type == "IB"){
            IOBNode* node  = new IOBNode(nodeId);
            adg_node = node;
        }else if(type == "OB"){
            IOBNode* node  = new IOBNode(nodeId);
            adg_node = node;
        }
        adg_node->setType(type);
        adg_node->setBitWidth(bitWidth);
        adg_node->setNumInputs(numInputs);
        adg_node->setNumOutputs(numOutputs);
        adg_node->setCfgBlkIdx(cfgBlkIdx);
        adg_node->setSubADG(subADG);
        if(attrs.count("configuration")){
            for(auto& elem : attrs["configuration"].items()){
                int subModuleId = std::stoi(elem.key());
                auto& info = elem.value();
                CfgDataLoc cfg;
                cfg.high = info[1].get<int>();
                cfg.low = info[2].get<int>();
                adg_node->addConfigInfo(subModuleId, cfg);
            }
        }
    }else{ // common components: ALU, Muxn, RF, DelayPipe, Const
        adg_node = new ADGNode(nodeId);
        adg_node->setType(type);
    } 
    return adg_node;
}


// parse ADGNode from instances json object, 
// modules<moduleId, <ADGNode*, used>>,  
ADGNode* ADGIR::parseADGNode(json& nodeJson, std::map<int, std::pair<ADGNode*, bool>>& modules){
    std::string type = nodeJson["type"].get<std::string>();
    if(type == "This"){
        return nullptr;
    }
    int nodeId = nodeJson["id"].get<int>();
    int moduleId = nodeJson["module_id"].get<int>();
    ADGNode* adg_node;
    ADGNode* module = modules[moduleId].first;
    bool renewNode = modules[moduleId].second; // used, need to re-new ADGNode
    if(type == "GPE" || type == "GIB" || type == "IB" || type == "OB"){                
        if(renewNode){ // re-new ADGNode
            if(type == "GPE"){
                GPENode* node = new GPENode(nodeId);
                *node = *(dynamic_cast<GPENode*>(module));
                adg_node = node;
            }else if(type == "GIB"){
                GIBNode* node  = new GIBNode(nodeId);
                *node = *(dynamic_cast<GIBNode*>(module));
                adg_node = node;
            }else if(type == "IB"){
                IOBNode* node  = new IOBNode(nodeId);
                *node = *(dynamic_cast<IOBNode*>(module));
                adg_node = node;
            }else if(type == "OB"){
                IOBNode* node  = new IOBNode(nodeId);
                *node = *(dynamic_cast<IOBNode*>(module));
                adg_node = node;
            }
            ADG* subADG = new ADG(); 
            *subADG = *(adg_node->subADG()); // COPY Sub-ADG
            adg_node->setSubADG(subADG);
        }else{ // reuse the ADGNode in modules
            adg_node = module;
            modules[moduleId].second = true;
        }
        if(type == "GIB"){
            bool trackReged = nodeJson["track_reged"].get<bool>(); 
            dynamic_cast<GIBNode*>(adg_node)->setTrackReged(trackReged);
        }
        int cfgBlkIdx = nodeJson["cfg_blk_index"].get<int>();    
        int x = nodeJson["x"].get<int>(); 
        int y = nodeJson["y"].get<int>(); 
        adg_node->setCfgBlkIdx(cfgBlkIdx);  
        adg_node->setX(x);     
        adg_node->setY(y);          
    }else{ // common components: ALU, Muxn, RF, DelayPipe, Const
        if(renewNode){ // re-new ADGNode
            adg_node = new ADGNode(nodeId);
            *adg_node = *module;  
        }else{ // reuse the ADGNode in modules
            adg_node = module;
            modules[moduleId].second = true;
        }      
    } 
    adg_node->setId(nodeId);
    adg_node->setType(type);
    return adg_node;
}


// parse ADGLink json object
void ADGIR::parseADGLinks(ADG* adg, json& linkJson){
    // std::cout << "Parse ADG Link" << std::endl;
    for(auto& elem : linkJson.items()){
        int linkId = std::stoi(elem.key());
        auto& link = elem.value();
        int srcId = link[0].get<int>();
        // std::string srcType = link[1].get<std::string>();
        int srcPort = link[2].get<int>();
        int dstId = link[3].get<int>();
        // std::string dstType = link[4].get<std::string>();
        int dstPort = link[5].get<int>();
        ADGLink* adg_link = new ADGLink(srcId, dstId);
        adg_link->setId(linkId);
        adg_link->setSrcId(srcId);
        adg_link->setDstId(dstId);
        adg_link->setSrcPortIdx(srcPort);
        adg_link->setDstPortIdx(dstPort);
        adg->addLink(linkId, adg_link);
    }
}


// analyze the connections among the internal sub-modules for GPENode
// fill _operandInputs 
void ADGIR::analyzeIntraConnect(GPENode* node){
    ADG* subAdg = node->subADG();
    int opeIdx; // ALU operand index
    for(auto& elem : subAdg->inputs()){
        auto input = elem.second.begin(); // one input only connected to one sub-module
        ADGNode* subNode = subAdg->node(input->first);
        while (subNode->type() != "ALU"){
            auto out = subNode->output(0).begin(); // submodule should only have one output in GPE
            subNode = subAdg->node(out->first);
            opeIdx = out->second;
        }
        // opeIdx is ALU operand index now
        node->addOperandInputs(opeIdx, elem.first);
    }
}


// analyze the connections among the internal sub-modules for GIBNode
// fill _out2ins, _in2outs 
void ADGIR::analyzeIntraConnect(GIBNode* node){
    ADG* subAdg = node->subADG();
    for(auto& ielem : subAdg->inputs()){
        int inPort = ielem.first;
        for(auto& subNode : ielem.second){
            int outPort;
            if(subNode.first == subAdg->id()){ // input directly connected to output
                outPort = subNode.second;
            } else {
                ADGNode* subNodePtr = subAdg->node(subNode.first);
                outPort = subNodePtr->output(0).begin()->second; // only one layer of Muxn
            }
            node->addIn2outs(inPort, outPort);
            node->addOut2ins(outPort, inPort);
        }
    }
}


// analyze the connections among the internal sub-modules for IOBNode
// fill _out2ins, _in2outs 
void ADGIR::analyzeIntraConnect(IOBNode* node){
    ADG* subAdg = node->subADG();
    for(auto& ielem : subAdg->inputs()){
        int inPort = ielem.first;
        int outPort;
        for(auto& subNode : ielem.second){
            if(subNode.first == subAdg->id()){ // input directly connected to output
                outPort = subNode.second;
            } else {
                ADGNode* subNodePtr = subAdg->node(subNode.first);
                outPort = subNodePtr->output(0).begin()->second; // only one layer of Muxn
            }           
            node->addIn2outs(inPort, outPort);
            node->addOut2ins(outPort, inPort);
        }
    }
}


// analyze if there are registers in the output ports of GIB
// fill _outReged
void ADGIR::analyzeOutReg(ADG* adg, GIBNode* node){
    for(auto& elem : node->outputs()){
        int id = elem.second.begin()->first; // GIB output port only connected to one node
        if(node->trackReged() && (adg->node(id)->type() == "GIB")){ // this edge is track
            node->setOutReged(elem.first, true);
        }else{
            node->setOutReged(elem.first, false);
        }
    }
}


// post-process the ADG nodes
void ADGIR::postProcess(ADG* adg){
    int numGpeNodes = 0;
    for(auto& node : adg->nodes()){
        auto nodePtr = node.second;
        if(nodePtr->type() == "GPE"){
            numGpeNodes++;
            analyzeIntraConnect(dynamic_cast<GPENode*>(nodePtr));
        } else if(nodePtr->type() == "GIB"){
            analyzeIntraConnect(dynamic_cast<GIBNode*>(nodePtr));
            analyzeOutReg(adg, dynamic_cast<GIBNode*>(nodePtr));
        } else if(nodePtr->type() == "IB" || nodePtr->type() == "OB"){
            analyzeIntraConnect(dynamic_cast<IOBNode*>(nodePtr));
        }
    }
    adg->setNumGpeNodes(numGpeNodes);
}