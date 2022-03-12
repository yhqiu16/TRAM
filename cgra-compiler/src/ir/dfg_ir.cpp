
#include "ir/dfg_ir.h"


DFGIR::DFGIR(std::string filename)
{
    _dfg = parseDFG(filename);
}

DFGIR::~DFGIR()
{
    if(_dfg){
        delete _dfg;
    }
}

// get node ID according to name
int DFGIR::nodeId(std::string name){
    if(_nodeName2id.count(name)){
        return _nodeName2id[name];
    } else{
        return -1;
    }
}


void DFGIR::setNodeId(std::string name, int id){
    _nodeName2id[name] = id;
}

// get input index according to name
int DFGIR::inputIdx(std::string name){
    if(_inputName2idx.count(name)){
        return _inputName2idx[name];
    } else{
        return -1;
    }
}


void DFGIR::setInputIdx(std::string name, int idx){
    _inputName2idx[name] = idx;
}

// get output index according to name
int DFGIR::outputIdx(std::string name){
    if(_outputName2idx.count(name)){
        return _outputName2idx[name];
    } else{
        return -1;
    }
}

void DFGIR::setOutputIdx(std::string name, int idx){
    _outputName2idx[name] = idx;
}

// get constant value according to name
uint64_t DFGIR::constValue(std::string name){
    if(_name2Consts.count(name)){
        return _name2Consts[name];
    } else{
        return 0;
    }
}

bool DFGIR::isConst(std::string name){
    return _name2Consts.count(name);
}


void DFGIR::setConst(std::string name, uint64_t value){
    _name2Consts[name] = value;
}


// get input index according to id
int DFGIR::inputIdx(int id){
    if(_inputId2idx.count(id)){
        return _inputId2idx[id];
    } else{
        return -1;
    }
}


void DFGIR::setInputIdx(int id, int idx){
    _inputId2idx[id] = idx;
}

// get output index according to id
int DFGIR::outputIdx(int id){
    if(_outputId2idx.count(id)){
        return _outputId2idx[id];
    } else{
        return -1;
    }
}

void DFGIR::setOutputIdx(int id, int idx){
    _outputId2idx[id] = idx;
}

// get constant value according to id
uint64_t DFGIR::constValue(int id){
    if(_id2Consts.count(id)){
        return _id2Consts[id];
    } else{
        return 0;
    }
}

bool DFGIR::isConst(int id){
    return _id2Consts.count(id);
}


void DFGIR::setConst(int id, uint64_t value){
    _id2Consts[id] = value;
}

// deprecated!!!
DFG* DFGIR::parseDFGDot(std::string filename){
    std::ifstream ifs(filename);
    if(!ifs){
        std::cout << "Cannot open DFG file: " << filename << std::endl;
        exit(1);
    }
    DFG* dfg = new DFG();
    dfg->setId(0); // DFG id = 0, node id = 1,...,n
    std::string line;
    int edgeIdx = 0;
    std::stringstream edge_stream;
    while (getline(ifs, line))
    {
        remove(line.begin(), line.end(), ' ');
        if(line.empty() || line.substr(0, 2) == "//"){
            continue;
        }
        int idx0 = line.find("[");
        int idx1 = line.find("=");
        int idx2 = line.find("]");
        int idx3 = line.find("->");
        if (idx0 != std::string::npos && idx3 == std::string::npos) { //nodes
            std::string nodeName = line.substr(0, idx0);
            std::string opName = line.substr(idx1 + 1, idx2-idx1-1);
            std::transform(opName.begin(), opName.end(), opName.begin(), toupper);
            int id = _nodeName2id.size() + 1;
            setNodeId(nodeName, id);
            if(opName == "INPUT"){
                int idx = _inputName2idx.size();
                setInputIdx(nodeName, idx);
                dfg->setInputName(idx, nodeName);
            } else if(opName == "OUTPUT"){
                int idx = _outputName2idx.size();
                setOutputIdx(nodeName, idx);
                dfg->setOutputName(idx, nodeName);
            } else if(opName == "CONST"){
                setConst(nodeName, 0); // ???????????????? PARSE CONST VALUE
            } else{
                DFGNode* dfg_node = new DFGNode();
                dfg_node->setId(id);
                dfg_node->setName(nodeName);
                dfg_node->setOperation(opName);
                dfg->addNode(dfg_node);
            }
        }
    }
    // rescan the file to parse the edges, making sure all the nodes are already parsed
    ifs.clear();
    ifs.seekg(0);
    while (getline(ifs, line))
    {
        remove(line.begin(), line.end(), ' ');
        if(line.empty() || line.substr(0, 2) == "//"){
            continue;
        }
        int idx0 = line.find("[");
        int idx1 = line.find("=");
        int idx2 = line.find("]");
        int idx3 = line.find("->");
        if (idx0 != std::string::npos && idx3 != std::string::npos) { // edges            
            std::string srcName = line.substr(0, idx3);
            std::string dstName = line.substr(idx3+2, idx0-idx3-2);
            int dstPort = std::stoi(line.substr(idx1+1, idx2-idx1-1));
            int srcPort = 0; // default one output for each node
            if(isConst(srcName)){ // merge const node into the node connected to it
                DFGNode* node = dfg->node(nodeId(dstName));
                node->setImm(constValue(srcName));
                node->setImmIdx(dstPort);
            } else{
                DFGEdge* edge = new DFGEdge(edgeIdx++);
                if(inputIdx(srcName) >= 0){ // INPUT                        
                    edge->setEdge(0, inputIdx(srcName), nodeId(dstName), dstPort);                    
                } else if(outputIdx(dstName) >= 0){ // output
                    edge->setEdge(nodeId(srcName), srcPort, 0, outputIdx(dstName));
                } else{
                    edge->setEdge(nodeId(srcName), srcPort, nodeId(dstName), dstPort);
                }
                dfg->addEdge(edge);
            }         
        }
    }
    return dfg;
}

// Json file transformed from dot file using graphviz
DFG* DFGIR::parseDFGJson(std::string filename){
    std::ifstream ifs(filename);
    if(!ifs){
        std::cout << "Cannot open DFG file: " << filename << std::endl;
        exit(1);
    }
    json dfgJson;
    ifs >> dfgJson;
    DFG* dfg = new DFG();
    dfg->setId(0); // DFG id = 0, node id = 1,...,n
    // parse nodes
    for(auto& nodeJson : dfgJson["objects"]){
        std::string nodeName = nodeJson["name"].get<std::string>();
        std::string opName = nodeJson["opcode"].get<std::string>();
        std::transform(opName.begin(), opName.end(), opName.begin(), toupper);
        int id = nodeJson["_gvid"].get<int>() + 1; // start from 1
        if(opName == "INPUT"){
            int idx = _inputId2idx.size();
            setInputIdx(id, idx);
            dfg->setInputName(idx, nodeName);
        } else if(opName == "OUTPUT"){
            int idx = _outputId2idx.size();
            setOutputIdx(id, idx);
            dfg->setOutputName(idx, nodeName);
        } else if(opName == "CONST"){
            uint64_t value = 0;
            if(nodeJson.contains("value")){
                value = std::stoull(nodeJson["value"].get<std::string>());
            }
            setConst(id, value); 
        } else{
            DFGNode* dfg_node = new DFGNode();
            dfg_node->setId(id);
            dfg_node->setName(nodeName);
            dfg_node->setOperation(opName);
            dfg->addNode(dfg_node);
        }
    }
    // parse edges
    for(auto& edgeJson : dfgJson["edges"]){
        int srcId = edgeJson["tail"].get<int>() + 1;
        int dstId = edgeJson["head"].get<int>() + 1;
        int dstPort;
        int srcPort; // default one output for each node
        if(edgeJson.contains("operand")){
            dstPort = std::stoi(edgeJson["operand"].get<std::string>());
        }else if(edgeJson.contains("headport")){
            auto str = edgeJson["headport"].get<std::string>(); // in0, in1...
            dstPort = std::stoi(str.substr(2, 1));
        }else if(outputIdx(dstId) < 0){ // not annotate dst-port, not output port
            DFGNode* node = dfg->node(dstId);
            dstPort = node->numInputs();
        }
        if(edgeJson.contains("tailport")){
            auto str = edgeJson["tailport"].get<std::string>(); // out0, out1...
            srcPort = std::stoi(str.substr(3, 1));
        }else{ // default one output for each node
            srcPort = 0;
        }
        if(isConst(srcId)){ // merge const node into the node connected to it
            DFGNode* node = dfg->node(dstId);
            node->setImm(constValue(srcId));
            node->setImmIdx(dstPort);
        } else{
            int edgeId = edgeJson["_gvid"].get<int>();
            DFGEdge* edge = new DFGEdge(edgeId);
            if(inputIdx(srcId) >= 0){ // INPUT                        
                edge->setEdge(0, inputIdx(srcId), dstId, dstPort);                    
            } else if(outputIdx(dstId) >= 0){ // output
                edge->setEdge(srcId, srcPort, 0, outputIdx(dstId));
            } else{
                edge->setEdge(srcId, srcPort, dstId, dstPort);
            }
            dfg->addEdge(edge);
        }         
    }
    return dfg;
}


DFG* DFGIR::parseDFG(std::string filename, std::string format){
    if(format == "dot"){
        return parseDFGDot(filename);
    }else{
        return parseDFGJson(filename);
    }
}