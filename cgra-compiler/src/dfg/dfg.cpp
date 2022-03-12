
#include "dfg/dfg.h"

DFG::DFG(){}

DFG::~DFG()
{
    for(auto& elem : _nodes){
        delete elem.second;
    }
    for(auto& elem : _edges){
        delete elem.second;
    }
}


void DFG::setInputName(int index, std::string name){
    _inputNames[index] = name;
}

std::string DFG::inputName(int index){
    if(_inputNames.count(index)){
        return _inputNames[index];
    }else{
        return "";
    }
}

void DFG::setOutputName(int index, std::string name){
    _outputNames[index] = name;
}

std::string DFG::outputName(int index){
    if(_outputNames.count(index)){
        return _outputNames[index];
    }else{
        return "";
    }
}

// return set<node-id, node-port-idx>
std::set<std::pair<int, int>> DFG::input(int index){
    if(_inputs.count(index)){
        return _inputs[index];
    }else{
        return {};
    }
}

// return set<node-id, node-port-idx>
std::pair<int, int> DFG::output(int index){
    if(_outputs.count(index)){
        return _outputs[index];
    }else{
        return {}; // std::make_pair(-1, -1); // return empty set
    }
}

// add input
void DFG::addInput(int index, std::pair<int, int> node){
    _inputs[index].emplace(node);
}

// add output
void DFG::addOutput(int index, std::pair<int, int> node){
    _outputs[index] = node;
}


void DFG::delInput(int index, std::pair<int, int> node){
    _inputs[index].erase(node);
}

void DFG::delInput(int index){
    _inputs.erase(index);
} 

void DFG::delOutput(int index){
    _outputs.erase(index);
}

// return edge id
std::set<int> DFG::inputEdge(int index){
    if(_inputEdges.count(index)){
        return _inputEdges[index];
    }else{
        return {}; // return empty set
    }
}

// return set<edge-id>
int DFG::outputEdge(int index){
    if(_outputEdges.count(index)){
        return _outputEdges[index];
    }else{
        return -1; 
    }
}

// add input edge
void DFG::addInputEdge(int index, int edgeId){
    _inputEdges[index].emplace(edgeId);
}

// add output edge
void DFG::addOutputEdge(int index, int edgeId){
    _outputEdges[index] = edgeId;
}

// delete input edge
void DFG::delInputEdge(int index, int edgeId){
    _inputEdges[index].erase(edgeId);
}

// delete input edge
void DFG::delInputEdge(int index){
    _inputEdges.erase(index);
}

// delete output edge
void DFG::delOutputEdge(int index){
    _outputEdges.erase(index);
}


DFGNode* DFG::node(int id){
    if(_nodes.count(id)){
        return _nodes[id];
    } else {
        return nullptr;
    }  
}


DFGEdge* DFG::edge(int id){
    if(_edges.count(id)){
        return _edges[id];
    } else {
        return nullptr;
    }  
}


void DFG::addNode(DFGNode* node){
    int id = node->id();
    _nodes[id] = node;
}


void DFG::addEdge(DFGEdge* edge){
    int id = edge->id();
    _edges[id] = edge;
    int srcId = edge->srcId();
    int dstId = edge->dstId();
    int srcPort = edge->srcPortIdx();
    int dstPort = edge->dstPortIdx();
    if(srcId == _id){ // source is input port
        addInput(srcPort, std::make_pair(dstId, dstPort));
        addInputEdge(srcPort, id);
    } else {
        DFGNode* src = node(srcId);
        assert(src);
        src->addOutput(srcPort, std::make_pair(dstId, dstPort));
        src->addOutputEdge(srcPort, id);
    }
    if(dstId == _id){ // destination is output port
        addOutput(dstPort, std::make_pair(srcId, srcPort));
        addOutputEdge(dstPort, id);
    } else{        
        DFGNode* dst = node(dstId);
        assert(dst);
        dst->addInput(dstPort, std::make_pair(srcId, srcPort));
        dst->addInputEdge(dstPort, id);
    }
}


void DFG::delNode(int id){
    DFGNode* dfgNode = node(id);
    for(auto& elem : dfgNode->inputEdges()){
        delEdge(elem.second);
    }
    for(auto& elem : dfgNode->outputEdges()){
        for(auto eid : elem.second){
            delEdge(eid);
        }        
    }
    _nodes.erase(id);
    delete dfgNode;
}


void DFG::delEdge(int id){
    DFGEdge* e = edge(id);
    int srcId = e->srcId();
    int dstId = e->dstId();
    int srcPortIdx = e->srcPortIdx();
    int dstPortIdx = e->dstPortIdx();
    if(srcId == _id){
        delInputEdge(srcPortIdx, id);
        delInput(srcPortIdx, std::make_pair(dstId, dstPortIdx));
    }else{
        DFGNode* srcNode = node(srcId);       
        srcNode->delOutputEdge(srcPortIdx, id);
        srcNode->delOutput(srcPortIdx, std::make_pair(dstId, dstPortIdx));
    }
    if(dstId == _id){
        delOutputEdge(dstPortIdx);
        delOutput(dstPortIdx);
    }else{
        DFGNode* dstNode = node(dstId);
        dstNode->delInputEdge(dstPortIdx);
        dstNode->delInput(dstPortIdx);
    }
    _edges.erase(id);
    delete e;
}


// sort dfg nodes in topological order
// depth-first search
void DFG::dfs(DFGNode* node, std::map<int, bool>& visited){
    int nodeId = node->id();
    if(visited.count(nodeId) && visited[nodeId]){
        return; // already visited
    }
    visited[nodeId] = true;
    for(auto& in : node->inputs()){
        int inNodeId = in.second.first;
        if(inNodeId == _id){ // node connected to DFG input port
            continue;
        }
        dfs(_nodes[inNodeId], visited); // visit input node
    }
    _topoNodes.push_back(_nodes[nodeId]);
}

// sort dfg nodes in topological order
void DFG::topoSortNodes(){
    _topoNodes.clear();
    std::map<int, bool> visited; // node visited status
    for(auto& out : outputs()){
        int outNodeId = out.second.first;
        if(outNodeId == _id){ // DFG input-output port passthrough
            continue;
        }
        dfs(_nodes[outNodeId], visited); // visit output node
    }
}


// // calculate the node timing, including
// // max latency of the input ports
// // latency mismatch among the operands
// void DFG::calNodesTiming(){
//     int maxLatMis = 0; // max latency mismatch among the operands of one DFG node
//     // calculate the bounds in topological order
//     for(DFGNode* node : topoNodes()){
//         int minLat = 0x7fffffff;
//         int maxLat = 0;
//         for(auto& elem : node->inputs()){
//             int srcNodeId = elem.second.first;
//             int srcNodeLat = 0;
//             int srcNodeOpLat = 0; // operation latency
//             if(srcNodeId != id()){ // connected to DFG input port
//                 srcNodeLat = _nodesTiming[srcNodeId].lat;
//                 srcNodeOpLat = this->node(srcNodeId)->opLatency();
//             }
//             int lat = srcNodeLat + srcNodeOpLat; // edge no latency
//             minLat = std::min(minLat, lat);
//             maxLat = std::max(maxLat, lat);
//         }
//         _nodesTiming[node->id()].lat = maxLat;
//         int diff = std::max(maxLat-minLat, 0);
//         _nodesTiming[node->id()].latMis = diff;
//         maxLatMis = std::max(maxLatMis, diff);         
//     }
//     _maxLatMis = maxLatMis;
// }


// ====== operators >>>>>>>>>>
// DFG copy
DFG& DFG::operator=(const DFG& that){
    if(this == &that) return *this;
    this->_id = that._id;
    this->_bitWidth = that._bitWidth;
    this->_inputNames = that._inputNames;
    this->_outputNames = that._outputNames;
    this->_inputs = that._inputs;
    this->_outputs = that._outputs;
    this->_inputEdges = that._inputEdges;
    this->_outputEdges = that._outputEdges;
    this->_topoNodes = that._topoNodes;
    // this->_maxLatMis = that._maxLatMis;
    // this->_nodesTiming = that._nodesTiming;
    // this->_nodes = that._nodes;
    for(auto& elem : that._nodes){
        int id = elem.first;
        DFGNode* node = new DFGNode();
        *node = *(elem.second);
        this->_nodes[id] = node;
    }
    // this->_edges = that._edges;
    for(auto& elem : that._edges){
        int id = elem.first;
        DFGEdge* edge = new DFGEdge();
        *edge = *(elem.second);
        this->_edges[id] = edge;
    }
    return *this;
}



void DFG::print(){
    std::cout << "DFG(id): " << _id << std::endl;
    std::cout << "bitWidth: " << _bitWidth << std::endl;
    std::cout << "numInputs: " << numInputs() << std::endl;
    std::cout << "numOutputs: " << numOutputs() << std::endl;
    std::cout << "inputNames: " << std::endl;
    for(auto& elem : _inputNames){
        std::cout << elem.first << ": " << elem.second << std::endl;
    }
    std::cout << "outputNames: " << std::endl;
    for(auto& elem : _outputNames){
        std::cout << elem.first << ": " << elem.second << std::endl;
    }
    std::cout << "inputs: " << std::endl;   
    for(auto& elem : _inputs){
        int idx = elem.first;
        auto& s = elem.second;
        for(auto it = s.begin(); it != s.end(); it++)
            std::cout << idx << ", " << it->first << ", " << it->second << std::endl;
    }
    std::cout << "outputs: " << std::endl;
    for(auto& elem : _outputs){
        std::cout << elem.first << ", " << elem.second.first << ", " << elem.second.second << std::endl;
    }
    for(auto& elem : _nodes){
        elem.second->print();
    }
}



