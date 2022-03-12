#ifndef __DFG_NODE_H__
#define __DFG_NODE_H__

#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <assert.h>
#include "op/operations.h"


class DFGNode
{
private:
    int _id;
    std::string _name;
    std::string _type;
    std::string _operation;
    int _bitWidth;
    // int _numInputs;
    // int _numOutputs;
    int _opLatency = 1; // operation latency
    bool _commutative; // if the inputs(operands) are commutative
    uint64_t _imm; // immedaite operand
    int _immIdx = -1;   // immedaite operand index, -1: no immediate 
    std::map<int, std::pair<int, int>> _inputs; // <input-index, <node-id, node-port-idx>>
    std::map<int, std::set<std::pair<int, int>>> _outputs; // <output-index, set<node-id, node-port-idx>>
    std::map<int, int> _inputEdges; // <input-index, edge-id>
    std::map<int, std::set<int>> _outputEdges; // <output-index, set<edge-id>>
public:
    DFGNode(){}
    ~DFGNode(){}
    int id(){ return _id; }
    void setId(int id){ _id = id; }
    std::string name(){ return _name; }
    void setName(std::string name){ _name = name; }
    std::string type(){ return _type; }
    void setType(std::string type){ _type = type; }
    std::string operation(){ return _operation; }
    // set operation, latency, commutative according to operation name
    void setOperation(std::string operation);
    int bitWidth(){ return _bitWidth; }
    void setBitWidth(int bitWidth){ _bitWidth = bitWidth; }
    int numInputs(){ return hasImm()? _inputs.size()+1 : _inputs.size(); }
    // void setNumInputs(int numInputs){ _numInputs = numInputs; }
    int numOutputs(){ return _outputs.size(); }
    // void setNumOutputs(int numOutputs){ _numOutputs = numOutputs; }
    void setOpLatency(int opLat){ _opLatency = opLat; }
    int opLatency(){ return _opLatency; }
    bool commutative(){ return _commutative; }
    void setCommutative(bool cmu){ _commutative = cmu; }
    uint64_t imm(){ return _imm; }
    void setImm(uint64_t imm){ _imm = imm; }
    int immIdx(){ return _immIdx; }
    void setImmIdx(int immIdx){ _immIdx = immIdx; }
    bool hasImm(){ return _immIdx >= 0; }
    const std::map<int, std::pair<int, int>>& inputs(){ return _inputs; }
    const std::map<int, std::set<std::pair<int, int>>>& outputs(){ return _outputs; }
    std::pair<int, int> input(int index); // return <node-id, node-port-idx>
    std::set<std::pair<int, int>> output(int index); // return set<node-id, node-port-idx>
    void addInput(int index, std::pair<int, int> node);  // add input
    void addOutput(int index, std::pair<int, int> node); // add output
    void delInput(int index);  // delete input
    void delOutput(int index, std::pair<int, int> node); // delete output
    void delOutput(int index); // delete output
    const std::map<int, int>& inputEdges(){ return _inputEdges; }
    const std::map<int, std::set<int>>& outputEdges(){ return _outputEdges; }
    int inputEdge(int index); // return edge id
    std::set<int> outputEdge(int index); // return set<edge-id>
    void addInputEdge(int index, int edgeId);  // add input edge
    void addOutputEdge(int index, int edgeId); // add output edge
    void delInputEdge(int index); // delete input edge
    void delOutputEdge(int index, int edgeId); // delete output edge
    void delOutputEdge(int index); // delete output edge
    virtual void print();
};





#endif