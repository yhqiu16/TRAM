#ifndef __ADG_H__
#define __ADG_H__

#include "adg/adg_node.h"
#include "adg/adg_link.h"



class ADG
{
private:
    int _id; // "This" sub-module ID
    int _bitWidth;
    int _numInputs;
    int _numOutputs;   
    int _numGpeNodes;
    int _cfgDataWidth;
    int _cfgAddrWidth;
    int _cfgBlkOffset;
    std::vector<uint64_t> _cfgBits;
    std::map<int, std::set<std::pair<int, int>>> _inputs; // <input-index, set<node-id, node-port-idx>>
    std::map<int, std::pair<int, int>> _outputs; // <output-index, <node-id, node-port-idx>>
    // std::map<int, bool> _input_used;  // <input-index, input-used>
    // std::map<int, bool> _output_used; // <output-index, output-used>
    std::map<int, ADGNode*> _nodes;   // <node-id, node>
    std::map<int, ADGLink*> _links;   // <link-id, link>
    // get configuration for ADG NOdes
    // define functions here, since the subADG in ADG node needs to be accessed
    // CfgData getCfgData4GPE()
public:
    ADG();
    ~ADG();
    int id(){ return _id; }
    void setId(int id){ _id = id; }
    int bitWidth(){ return _bitWidth; }
    void setBitWidth(int bitWidth){ _bitWidth = bitWidth; }
    int numInputs(){ return _numInputs; }
    void setNumInputs(int numInputs){ _numInputs = numInputs; }
    int numOutputs(){ return _numOutputs; }
    void setNumOutputs(int numOutputs){ _numOutputs = numOutputs; }
    int numGpeNodes(){ return _numGpeNodes; }
    void setNumGpeNodes(int numGpeNodes){ _numGpeNodes = numGpeNodes; }
    int cfgDataWidth(){ return _cfgDataWidth; }
    void setCfgDataWidth(int cfgDataWidth){ _cfgDataWidth = cfgDataWidth; }
    int cfgAddrWidth(){ return _cfgAddrWidth; }
    void setCfgAddrWidth(int cfgAddrWidth){ _cfgAddrWidth = cfgAddrWidth; }
    int cfgBlkOffset(){ return _cfgBlkOffset; }
    void setCfgBlkOffset(int cfgBlkOffset){ _cfgBlkOffset = cfgBlkOffset; }
    const std::map<int, std::set<std::pair<int, int>>>& inputs(){ return _inputs; }
    const std::map<int, std::pair<int, int>>& outputs(){ return _outputs; }
    std::set<std::pair<int, int>> input(int index); // return <node-id, node-port-idx>
    std::pair<int, int> output(int index); // return set<node-id, node-port-idx>
    void addInput(int index, std::pair<int, int> node); // add input
    void addOutput(int index, std::pair<int, int> node); // add output
    // bool inputUsed(int index);
    // bool outputUsed(int index);
    // void setInputUsed(int index, bool used); 
    // void setOutputUsed(int index, bool used);
    const std::map<int, ADGNode*>& nodes(){ return _nodes; }
    const std::map<int, ADGLink*>& links(){ return _links; }
    ADGNode* node(int id);
    ADGLink* link(int id);
    void addNode(int id, ADGNode* node);
    void addLink(int id, ADGLink* link);
    void delNode(int id);
    void delLink(int id);

    ADG& operator=(const ADG& that);

    void print();
    
};





#endif