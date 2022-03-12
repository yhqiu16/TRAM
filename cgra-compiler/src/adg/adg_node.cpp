
#include "adg/adg_node.h"


// ===================================================
//   ADGNode functions
// ===================================================

// return <node-id, node-port-idx>
std::pair<int, int> ADGNode::input(int index){
    if(_inputs.count(index)){
        return _inputs[index];
    }else{
        return {}; // std::make_pair(-1, -1);
    }
}

// return set<node-id, node-port-idx>
std::set<std::pair<int, int>> ADGNode::output(int index){
    if(_outputs.count(index)){
        return _outputs[index];
    }else{
        return {}; // return empty set
    }
}

// add input, _input_used
void ADGNode::addInput(int index, std::pair<int, int> node){
    _inputs[index] = node;
    // _input_used[index] = false; // initialize not used 
}

// add output, _output_used
void ADGNode::addOutput(int index, std::pair<int, int> node){
    _outputs[index].emplace(node);
    // _output_used[index] = false; // initialize not used 
}


// inputs connected to each output
std::set<int> ADGNode::out2ins(int outPort){
    if(_out2ins.count(outPort)){
        return _out2ins[outPort];
    } else {
        return {};
    }
}


// outputs connected to each input
std::set<int> ADGNode::in2outs(int inPort){
    if(_in2outs.count(inPort)){
        return _in2outs[inPort];
    } else {
        return {};
    }
}

// add input connected to the output
void ADGNode::addOut2ins(int outPort, int inPort){
    if(_inputs.count(inPort) && _outputs.count(outPort)){
        _out2ins[outPort].emplace(inPort);
    }
}

// add output connected to the input
void ADGNode::addIn2outs(int inPort, int outPort){
    if(_inputs.count(inPort) && _outputs.count(outPort)){
        _in2outs[inPort].emplace(outPort);
    }
}


// check if the input and output port are connected
bool ADGNode::isInOutConnected(int inPort, int outPort){
    if(!_out2ins.count(outPort)){
        return false;
    } else{
        for(auto in : _out2ins[outPort]){
            if(in == inPort){
                return true;
            }
        }
        return false;
    }
}


// get config info for sub-module
const CfgDataLoc& ADGNode::configInfo(int id){
    assert(_configInfo.count(id));
    return _configInfo[id];
}


// add config info for sub-module
void ADGNode::addConfigInfo(int id, CfgDataLoc subModuleCfg){
    _configInfo[id] = subModuleCfg;
}


void ADGNode::printData(){
    std::cout << "ADGNode(id): " << _id << std::endl;
    std::cout << "bitWidth: " << _bitWidth << std::endl;
    std::cout << "type: " << _type << std::endl;
    std::cout << "numInputs: " << _numInputs << std::endl;
    std::cout << "numOutputs: " << _numOutputs << std::endl;
    std::cout << "cfgBlkIdx: " << _cfgBlkIdx << std::endl;
    // std::cout << "cfgBitLen: " << _cfgBitLen << std::endl;
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
    std::cout << "out2ins: " << std::endl;
    for(auto& elem : _out2ins){
        for(auto sec : elem.second)
            std::cout << elem.first << ", " << sec << std::endl;
    }
    std::cout << "in2outs: " << std::endl;
    for(auto& elem : _in2outs){
        for(auto sec : elem.second)
            std::cout << elem.first << ", " << sec << std::endl;
    }
}


void ADGNode::print(){
    printData();
}


// ===================================================
//   GPENode functions
// ===================================================

// set numOperands and resize _operandInputs 
void GPENode::setNumOperands(int numOperands){ 
    _numOperands = numOperands; 
    _operandInputs.resize(numOperands);
}

// add supported operation
void GPENode::addOperation(std::string op){
    _operations.emplace(op);
}

// delete supported operation
void GPENode::delOperation(std::string op){
    _operations.erase(op);
}

// check if the operation is supported
bool GPENode::opCapable(std::string op){
    return _operations.count(op);
}

// get input ports connected to this operand
const std::set<int>& GPENode::operandInputs(int opeIdx){
    assert(_operandInputs.size() > opeIdx);
    return _operandInputs[opeIdx]; 
}

// add input port connected to this operand
void GPENode::addOperandInputs(int opeIdx, int inputIdx){
    assert(_operandInputs.size() > opeIdx);
    if(inputs().count(inputIdx)){
        _operandInputs[opeIdx].emplace(inputIdx);
    }
}


// delete input port connected to this operand
void GPENode::delOperandInputs(int opeIdx, int inputIdx){
    assert(_operandInputs.size() > opeIdx);
    if(inputs().count(inputIdx)){
        _operandInputs[opeIdx].erase(inputIdx);
    }
}


// add input ports connected to this operand
void GPENode::addOperandInputs(int opeIdx, std::set<int> inputIdxs){
    assert(_operandInputs.size() > opeIdx);
    _operandInputs[opeIdx] = inputIdxs;
}


// get which operand this input is connected
int GPENode::getOperandIdx(int inputIdx){
    for(int i = 0; i < _numOperands; i++){
        if(_operandInputs[i].count(inputIdx)){
            return i;
        }
    }
    return -1;
}


void GPENode::print(){
    printData();
    std::cout << "numOperands: " << _numOperands << std::endl;
    std::cout << "operandInputs: " << std::endl;
    int i = 0;
    for(auto& elem : _operandInputs){
        std::cout << i++ << ": ";
        for(auto in : elem){
            std::cout << in << " ";
        }
        std::cout << std::endl;
    }
}


// ===================================================
//   GIBNode functions
// ===================================================

// if there are registers in the output port 
bool GIBNode::outReged(int idx){
    return _outReged[idx];
}

// if there are registers in the output port 
void GIBNode::setOutReged(int idx, bool reged){
    _outReged[idx] = reged;
}


void GIBNode::print(){
    printData();
    std::cout << "trackReged: " << _trackReged << std::endl;
    std::cout << "outReged: " << std::endl;
    for(auto& elem : _outReged){
        std::cout << elem.first << ": " << elem.second << std::endl;
    }
}