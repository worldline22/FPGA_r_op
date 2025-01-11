#include <iomanip>
#include "global.h"
#include "wirelength.h"
#include "rsmt.h"


int reportWirelength() {
  int totalWirelengthBaseline = 0;
  int totalCritWirelengthBaseline = 0;
  int totalWirelengthOptimized = 0;
  int totalCritWirelengthOptimized = 0;
  for (auto iter : glbNetMap) {
    Net* net = iter.second;
    if (net->isClock()) {
      continue;
    }
    int incre;

    totalCritWirelengthBaseline += net->getCritWireLength(true);
    totalWirelengthBaseline += net->getNonCritWireLength(true);

    totalCritWirelengthOptimized += net->getCritWireLength(false);
    totalWirelengthOptimized += net->getNonCritWireLength(false); 
    // if (net->getNonCritWireLength(true) != net->getNonCritWireLength(false)) {
    //   std::cout << " Net " << net->getId() << " non-critical wirelength mismatch: baseline = " << net->getNonCritWireLength(true) << "; optimized = " << net->getNonCritWireLength(false) << std::endl;
    // }
  }

  // append critical wirelength to total wirelength
  totalWirelengthBaseline += totalCritWirelengthBaseline;
  totalWirelengthOptimized += totalCritWirelengthOptimized;

  double ratioBaseline = 100.0 * (double)totalCritWirelengthBaseline   / (double)totalWirelengthBaseline;
  double ratioOptimized = 100.0 * (double)totalCritWirelengthOptimized / (double)totalWirelengthOptimized;
  std::cout << "  Baseline wirelength: total = " << totalWirelengthBaseline << "; crit = " << totalCritWirelengthBaseline << " (" << std::setprecision(2) << ratioBaseline <<"%)" << std::endl;    
  std::cout << "  Optimized wirelength: total  = " << totalWirelengthOptimized << "; crit = " << totalCritWirelengthOptimized << " (" << std::setprecision(2) << ratioOptimized <<"%)" << std::endl; 
  std::cout << "  Baseline Non-crit: " << totalWirelengthBaseline - totalCritWirelengthBaseline << std::endl;
  std::cout << "  Optimized Non-crit: " << totalWirelengthOptimized - totalCritWirelengthOptimized << std::endl;
  std::cout << "  Baseline Eq Cost: " << totalWirelengthBaseline + totalCritWirelengthBaseline << std::endl;
  std::cout << "  Optimized Eq Cost: " << totalWirelengthOptimized + totalCritWirelengthOptimized << std::endl;
  
  return 0;
}