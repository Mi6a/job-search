#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <set>

#include "include/cpp20/unordered_set" // see test()
#include "include/cpp20/unordered_map" // see test()

#include "OrderCacheImpl.h"

using namespace std::string_literals;

using std::cout; using std::cin; using std::endl;
using std::string;
using std::vector;

using StrShared = std::shared_ptr<string>;

struct StrSharedEqual {
   using is_transparent = void; // for find() override

   bool operator()(const StrShared& ss1, const StrShared& ss2) const  noexcept {
      return *ss1 == *ss2;
   }
   bool operator()(const string& s1, const StrShared& ss2) const  noexcept {
      return s1 == *ss2;
   }
   bool operator()(const StrShared& ss1, const string& s2) const  noexcept {
      return *ss1 == s2;
   }
};

//
struct StrSharedHash {
   using is_transparent = void; // for find() override

   std::size_t operator()(const StrShared& ss) const noexcept {
      return std::hash<string>{}(*ss); 
   }

   std::size_t operator()(const std::string& s) const noexcept {
      return std::hash<string>{}(s);
   }
};

using PtrStrSet = std::unordered_set<StrShared, StrSharedHash, StrSharedEqual>;

using PtrStrIntMap = std::unordered_map<StrShared, int, StrSharedHash, StrSharedEqual>;

//
int test()
{
   string s1 = "Serg"s;
   auto ss1 = std::make_shared<string>(s1);
   // testing set with cxx20
   PtrStrSet pss;
   auto resS = pss.insert(ss1);
   cout << "Inserted: " << (resS.second? "yes" : "no") << endl;
   auto foundS = pss.find(s1);
   cout << "Found: ";
   if (foundS != pss.end())
      cout << "yes: " << **foundS << endl;
   else
      cout << "no" << endl;

   // testing map with cxx20
   PtrStrIntMap msi1;
   auto resM = msi1.insert({ss1, 2});
   cout << "Inserted: " << (resM.second ? "yes" : "no") << endl;
   auto foundM = msi1.find(s1);
   cout << "Found: ";
   if (foundM != msi1.end())
      cout << "yes: " << (*foundM).second << endl;
   else
      cout << "no" << endl;

   return 0;
}

using VecStr = vector<string>;

// splitting the string to tokens
VecStr split(const string& str, const string& delim) {
   VecStr tokens;
   size_t prev = 0, pos = 0;
   do {
      pos = str.find(delim, prev);
      if (pos == string::npos)
         pos = str.length();
      string token = str.substr(prev, pos - prev);
      if (!token.empty())
         tokens.push_back(std::move(token));
      prev = pos + delim.length();
   }
   while (pos < str.length() && prev < str.length());
   return tokens;
}

using VecOrders = vector<Order>;

//
void printOrder(const Order& ord) {
   cout << ord.orderId() << "\t" << ord.securityId() << "\t" << ord.side() << "\t" << ord.qty() << "\t" << ord.user() << "\t" << ord.company() << endl;
}

//
void printOrders(const VecOrders& orders) {
   for (auto& ord : orders) {
      printOrder(ord);
   }
}

//
void getAndPrintOrders(const OrderCacheImpl& orders) {
   cout << "Orders: " << endl;
   VecOrders ordAll = orders.getAllOrders();
   printOrders(ordAll);
}

//
int main() {
   OrderCacheImpl orders;
   std::set<string> comps, users, secs;
   VecStr vecOrders;
   // read input from standard input
   string line;
   while (true) {
      std::getline(cin, line);
      // there is a confusion in text descrption and example input
      // lets tolerate for that
      if (line.empty())
         break;
      VecStr order = split(line, " "s);
      Order ord(order[0], order[1], order[2], std::stoul(order[3]), order[4], order[5]);
      orders.addOrder(ord);
      comps.insert(ord.company());
      users.insert(ord.user());
      secs.insert(ord.securityId());
      vecOrders.push_back(ord.orderId());
   }
   getAndPrintOrders(orders);
   // test getMatchingSizeForSecurity()
   for (auto& sec : secs) {
      unsigned qm = orders.getMatchingSizeForSecurity(sec);
      cout << "getMatchingSizeForSecurity: " << sec << " : " << qm << endl;
   }

   string secFst = *secs.begin();
   cout << "cancelOrdersForSecIdWithMinimumQty: " << secFst << ", 500" << endl;
   orders.cancelOrdersForSecIdWithMinimumQty(secFst, 500);
   getAndPrintOrders(orders);
   string ordFst = vecOrders.front();
   cout << "cancelOrder: " << ordFst << endl;
   orders.cancelOrder(ordFst);
   string userFst = *users.begin();
   cout << "cancelOrdersForUser: " << userFst << endl;
   orders.cancelOrdersForUser(*users.begin());
   getAndPrintOrders(orders);
}
