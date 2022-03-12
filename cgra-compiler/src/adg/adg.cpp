#include "adg/adg.h"

ADG::ADG(){}

ADG::~ADG()
{
    for(auto& elem : _nodes){
        auto node = elem.second;
        auto sub_adg = node->subADG();
        if(sub_adg){
            delete sub_adg;
        }
        delete node;
    }
    for(auto& elem : _links){
        delete elem.second;
    }
}


// return set<node-id, node-port-idx>
std::set<std::pair<int, int>> ADG::input(int index){
    if(_inputs.count(index)){
        return _inputs[index];
    }else{
        return {};
    }
}

// return set<node-id, node-port-idx>
std::pair<int, int> ADG::output(int index){
    if(_outputs.count(index)){
        return _outputs[index];
    }else{
        return {}; // std::make_pair(-1, -1); // return empty set
    }
}

// add input, _input_used
void ADG::addInput(int index, std::pair<int, int> node){
    _inputs[index].emplace(node);
    // _input_used[index] = false; // initialize not used 
}

// add output, _output_used
void ADG::addOutput(int index, std::pair<int, int> node){
    _outputs[index] = node;
    // _output_used[index] = false; // initialize not used 
}

// bool ADG::inputUsed(int index){
//     if(_input_used.count(index)){
//         return _input_used[index];
//     }else{
//         return true; // default used
//     }
// }


// bool ADG::outputUsed(int index){
//     if(_output_used.count(index)){
//         return _output_used[index];
//     }else{
//         return true; // default used
//     }
// }

// // also set other input according to _mutexInputSets
// void ADG::setInputUsed(int index, bool used){
//     _input_used[index] = used;
// } 

// // no _mutexOutputSets
// void ADG::setOutputUsed(int index, bool used){
//     _output_used[index] = used;
// }

ADGNode* ADG::node(int id){
    if(_nodes.count(id)){
        return _nodes[id];
    } else {
        return nullptr;
    }  
}


ADGLink* ADG::link(int id){
    if(_links.count(id)){
        return _links[id];
    } else {
        return nullptr;
    }  
}


void ADG::addNode(int id, ADGNode* node){
    _nodes[id] = node;
}


void ADG::addLink(int id, ADGLink* link){
    _links[id] = link;
    int srcId = link->srcId();
    int dstId = link->dstId();
    int srcPort = link->srcPortIdx();
    int dstPort = link->dstPortIdx();
    if(srcId == _id){ // source is input port
        addInput(srcPort, std::make_pair(dstId, dstPort));
    } else {
        ADGNode* src = node(srcId);
        assert(src);
        src->addOutput(srcPort, std::make_pair(dstId, dstPort));
    }
    if(dstId == _id){ // destination is output port
        addOutput(dstPort, std::make_pair(srcId, srcPort));
    } else{        
        ADGNode* dst = node(dstId);
        assert(dst);
        dst->addInput(dstPort, std::make_pair(srcId, srcPort));
    }
}


void ADG::delNode(int id){
    _nodes.erase(id);
}


void ADG::delLink(int id){
    _links.erase(id);
}


ADG& ADG::operator=(const ADG& that){
    if(this == &that) return *this;
    this->_id = that._id;
    this->_bitWidth = that._bitWidth;
    this->_numInputs = that._numInputs;
    this->_numOutputs = that._numOutputs;
    this->_cfgDataWidth = that._cfgDataWidth;
    this->_cfgAddrWidth = that._cfgAddrWidth;
    this->_cfgBlkOffset = that._cfgBlkOffset;
    this->_cfgBits = that._cfgBits;
    this->_inputs = that._inputs;
    this->_outputs = that._outputs;
    // this->_input_used = that._input_used;
    // this->_output_used = that._output_used;
    for(auto& elem : that._nodes){
        ADGNode* node = new ADGNode();
        *node = *(elem.second);
        this->_nodes[elem.first] = node;
    }
    for(auto& elem : that._links){
        ADGLink* link = new ADGLink();
        *link = *(elem.second);
        this->_links[elem.first] = link;
    }
    return *this;
}

void ADG::print(){
    std::cout << "ADG(id): " << _id << std::endl;
    std::cout << "bitWidth: " << _bitWidth << std::endl;
    std::cout << "numInputs: " << _numInputs << std::endl;
    std::cout << "numOutputs: " << _numOutputs << std::endl;
    std::cout << "cfgDataWidth: " << _cfgDataWidth << std::endl;
    std::cout << "cfgAddrWidth: " << _cfgAddrWidth << std::endl;
    std::cout << "cfgBlkOffset: " << _cfgBlkOffset << std::endl;
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