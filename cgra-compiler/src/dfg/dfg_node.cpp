
#include "dfg/dfg_node.h"

// ===================================================
//   DFGNode functions
// ===================================================

// set operation, latency, commutative according to operation name
void DFGNode::setOperation(std::string operation){ 
    if(!Operations::opCapable(operation)){
        std::cout << operation << " is not supported!" << std::endl;
        exit(1);
    }
    // TODO: add bitwidth check
    _operation = operation; 
    setOpLatency(Operations::latency(operation));
    setCommutative(Operations::isCommutative(operation));
}


// return <node-id, node-port-idx>
std::pair<int, int> DFGNode::input(int index){
    if(_inputs.count(index)){
        return _inputs[index];
    }else{
        return {}; // std::make_pair(-1, -1);
    }
}

// return set<node-id, node-port-idx>
std::set<std::pair<int, int>> DFGNode::output(int index){
    if(_outputs.count(index)){
        return _outputs[index];
    }else{
        return {}; // return empty set
    }
}


// add input
void DFGNode::addInput(int index, std::pair<int, int> node){
    _inputs[index] = node;
}

// add output
void DFGNode::addOutput(int index, std::pair<int, int> node){
    _outputs[index].emplace(node);
}

// delete input
void DFGNode::delInput(int index){
    _inputs.erase(index);
}

// delete output
void DFGNode::delOutput(int index, std::pair<int, int> node){
    _outputs[index].erase(node);
}

// delete output
void DFGNode::delOutput(int index){
    _outputs.erase(index);
}

// return edge id
int DFGNode::inputEdge(int index){
    if(_inputEdges.count(index)){
        return _inputEdges[index];
    }else{
        return -1; 
    }
}

// return set<edge-id>
std::set<int> DFGNode::outputEdge(int index){
    if(_outputEdges.count(index)){
        return _outputEdges[index];
    }else{
        return {}; // return empty set
    }
}

// add input edge
void DFGNode::addInputEdge(int index, int edgeId){
    _inputEdges[index] = edgeId;
}

// add output edge
void DFGNode::addOutputEdge(int index, int edgeId){
    _outputEdges[index].emplace(edgeId);
}

// delete input edge
void DFGNode::delInputEdge(int index){
    _inputEdges.erase(index);
}

// delete output edge
void DFGNode::delOutputEdge(int index, int edgeId){
    assert(_outputEdges.count(index));
    _outputEdges[index].erase(edgeId);
}

// delete output edge
void DFGNode::delOutputEdge(int index){
    _outputEdges.erase(index);
}


void DFGNode::print(){
    std::cout << "DFGNode(id): " << _id << std::endl;
    std::cout << "bitWidth: " << _bitWidth << std::endl;
    std::cout << "type: " << _type << std::endl;
    std::cout << "name: " << _name << std::endl;
    std::cout << "numInputs: " << numInputs() << std::endl;
    std::cout << "numOutputs: " << numOutputs() << std::endl;
    std::cout << "imm: " << _imm << ", immIdx: " << _immIdx << std::endl;
    std::cout << "commutative: " << _commutative << std::endl;
    std::cout << "inputs: " << std::endl;
    for(auto& elem : _inputs){
        std::cout << elem.first << ", " << elem.second.first << ", " << elem.second.second << std::endl;
    }
    std::cout << "outputs: " << std::endl;
    for(auto& elem : _outputs){
        int idx = elem.first;
        auto& s = elem.second;
        for(auto it = s.begin(); it != s.end(); it++)
            std::cout << idx << ", " << it->first << ", " << it->second << std::endl;
    }
}