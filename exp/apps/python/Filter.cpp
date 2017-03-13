#include "Filter.h"
#include "Galois/Statistic.h"
#include "Galois/Bag.h"

NodeList filterNode(Graph *g, const KeyAltTy key, const ValAltTy value) {
  Galois::StatManager statManager;
  Galois::InsertBag<GNode> bag;

  Galois::StatTimer T;
  T.start();

  Galois::do_all_local(
    *g,
    [g, &bag, key, value] (GNode n)
      {
        auto& data = (*g).getData(n);
        auto it = data.attr.find(key);
        if (it != data.attr.end() && it->second == value) {
          bag.push_back(n);
        }
      }
    );

  T.stop();

  int num = std::distance(bag.begin(), bag.end());
  GNode *l = NULL;
  if (num) {
    l = new GNode [num] ();
    int i = 0;
    for (auto n: bag) {
      l[i++] = n;
    }
  }

  return {num, l};
}

void deleteNodeList(NodeList nl) {
  delete[] nl.nodes;
}
