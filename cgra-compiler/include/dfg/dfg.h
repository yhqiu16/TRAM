#ifndef __DFG_H__
#define __DFG_H__

#include "dfg/dfg_node.h"
#include "dfg/dfg_edge.h"

struct DFGNodeTiming
{
    int lat; // max latency of the input ports
    int latMis; // latency mismatch among the operands 
};


class DFG
{
private:
    int _id; // "This"(INPUT/OUTPUT) ID
    int _bitWidth;
    // int _numInputs;
    // int _numOutputs;   
    std::map<int, std::string> _inputNames; // <input-index, input-port-name>
    std::map<int, std::string> _outputNames; // <input-index, input-port-name>
    std::map<int, std::set<std::pair<int, int>>> _inputs; // <input-index, set<node-id, node-port-idx>>
    std::map<int, std::pair<int, int>> _outputs; // <output-index, <node-id, node-port-idx>>
    std::map<int, std::set<int>> _inputEdges; // <input-index, set<edge-id>>
    std::map<int, int> _outputEdges; // <output-index, edge-id>
    std::map<int, DFGNode*> _nodes;   // <node-id, node>
    std::map<int, DFGEdge*> _edges;   // <edge-id, edge>

    // depth-first search, sort dfg nodes in topological order
    void dfs(DFGNode* node, std::map<int, bool>& visited);

protected:
    // DFG nodes in topological order, DFG should be DAG
    std::vector<DFGNode*> _topoNodes;

    // // max latency mismatch among the operands of one DFG node
    // int _maxLatMis;
    // // min latency bound of the DFG nodes, <node-id, DFGNodeTiming>
    // std::map<int, DFGNodeTiming> _nodesTiming;

public:
    DFG();
    ~DFG();
    int id(){ return _id; }
    void setId(int id){ _id = id; }
    int bitWidth(){ return _bitWidth; }
    void setBitWidth(int bitWidth){ _bitWidth = bitWidth; }
    int numInputs(){ return _inputs.size(); }
    // void setNumInputs(){_numInputs = _inputs.size(); }
    // void setNumInputs(int numInputs){ _numInputs = numInputs; }
    int numOutputs(){ return _outputs.size(); }
    // void setNumOutputs(){ _numOutputs = _outputs.size(); }
    // void setNumOutputs(int numOutputs){ _numOutputs = numOutputs; }
    void setInputName(int index, std::string name);
    std::string inputName(int index);
    void setOutputName(int index, std::string name);
    std::string outputName(int index);
    const std::map<int, std::set<std::pair<int, int>>>& inputs(){ return _inputs; }
    const std::map<int, std::pair<int, int>>& outputs(){ return _outputs; }
    std::set<std::pair<int, int>> input(int index); // return <node-id, node-port-idx>
    std::pair<int, int> output(int index); // return set<node-id, node-port-idx>
    void addInput(int index, std::pair<int, int> node);  // add input
    void addOutput(int index, std::pair<int, int> node); // add output
    void delInput(int index, std::pair<int, int> node);  // delete input
    void delInput(int index);  // delete input
    void delOutput(int index); // delete output
    const std::map<int, std::set<int>>& inputEdges(){ return _inputEdges; }
    const std::map<int, int>& outputEdges(){ return _outputEdges; }
    std::set<int> inputEdge(int index); // return set<edge-id>
    int outputEdge(int index); // return <edge-id>
    void addInputEdge(int index, int edgeId);  // add input edge
    void addOutputEdge(int index, int edgeId); // add output edge
    void delInputEdge(int index, int edgeId);  // delete input edge
    void delInputEdge(int index);  // delete input edge
    void delOutputEdge(int index); // delete output edge
    const std::map<int, DFGNode*>& nodes(){ return _nodes; }
    const std::map<int, DFGEdge*>& edges(){ return _edges; }
    DFGNode* node(int id);
    DFGEdge* edge(int id);
    void addNode(DFGNode* node);
    void addEdge(DFGEdge* edge);
    void delNode(int id);
    void delEdge(int id);

    // DFG nodes in topological order
    const std::vector<DFGNode*>& topoNodes(){ return _topoNodes; }
    // sort dfg nodes in topological order
    void topoSortNodes();

    // void setMaxLatMis(int maxLatMis){ _maxLatMis = maxLatMis; }
    // int maxLatMis(){ return _maxLatMis; }
    // const std::map<int, DFGNodeTiming>& nodesTiming(){ return _nodesTiming; }
    // // calculate the node timing, including
    // // max latency of the input ports
    // // latency mismatch among the operands
    // void calNodesTiming();

    // ====== operators >>>>>>>>>>
    // DFG copy
    DFG& operator=(const DFG& that);


    void print();
};




#endif