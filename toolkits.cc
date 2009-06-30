#include "toolkits.hpp"

namespace vina {

  void CallsiteOutput::initializeGraph(const string& graphname)
    {
      (*os_) << "digraph " << graphname << "{\n"
	     << "label=\"CallSite Graph\"\n"
	     << "edge [fontsize=8, fontname=\"Times-Italic\"]\n";
    }
  void CallsiteOutput::finalizeGraph()
  {
    (*os_) << "}"
	   << "#end of file";
  }
  void CallsiteOutput::enter(){
    stack_.push(temp_);
  }
  void CallsiteOutput::enterMT() {
    std::ostringstream oss;
    int gid = grp_++;
    oss << "cluster_" << gid;
    std::string sub = oss.str();
    
    (*os_) << "subgraph " << sub
	   << "{\n"
	   << "style=filled\n"
	   << "color=lightgray\n"
	   << "label=\"\"";
    }
  void CallsiteOutput::leave(){
    stack_.pop();
  }
  void CallsiteOutput::leaveMT() {
    (*os_) << "}\n";
  }
  // call after callsite(D) to link dependences
  // side-effect: clear out dep_
  void CallsiteOutput::depend() {
    for (auto i=dep_.begin(); i != dep_.end(); ++i)
      {
	(*os_) << *i << " -> " << temp_
	       << "[style=dotted]\n";
      }
    dep_.clear();
  }
  void CallsiteOutput::callsite(const string& target)
  {
    std::ostringstream oss;
    std::string tar;
    
    int tid = cnt_++; // get a id, invisible for dot graph
    oss << target << "_" << tid;
    tar = oss.str();
    
    (*os_) << tar << "[label=\""
	   << target << "\"]\n";
    
    if ( stack_.size() != 0 ){
      (*os_) << stack_.top() << " -> " << tar << "\n";
    }
    
    temp_ = tar;
    }
  void CallsiteOutput::callsiteD(const string& target)
  {
    callsite(target);
    dep_.push_back(temp_);
  }
  void CallsiteOutput::root(const string& root)
  {
    (*os_) << "subgraph cluster_root {\nlabel=\"\"";
    callsite(root);
    (*os_) << "}\n";
  }
}
