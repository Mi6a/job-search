#include "OrderCacheImpl.h"

using std::string;
using std::make_shared;
using namespace std::string_literals;

static const std::string strBuy{"Buy"s};
static const std::string strSell{"Sell"s};

//
void OrderCacheImpl::addOrder(Order order) {
   StrShared ss = make_shared<string>(order.orderId());
   auto orderRes = m_orders.insert({ss, OrderShared{}});
   if (!orderRes.second)
      return; // already exist
   OrderShared sh_ord = make_shared<OrderImpl>();
   orderRes.first->second = sh_ord;
   sh_ord->m_id = ss;
   // assign user
   // use same ss to avoid allocating many strings
   *ss = order.user();
   auto userRes = m_users.insert({ss, User{}});
   auto userIter = userRes.first;
   if (userRes.second) { // new user
      // have to search for the company
      *ss = order.company();
      auto compRes = m_companies.insert(ss);
      userIter->second._comp = *(compRes.first);
   }
   sh_ord->m_user = userIter->first;
   sh_ord->m_comp = userIter->second._comp;
   userIter->second._orders.insert(sh_ord);
   // assign security
   *ss = order.securityId();
   auto compRes = m_securs.insert({ss, OrderSet{}});
   auto compIter = compRes.first;
   sh_ord->m_sec = compIter->first;
   compIter->second.insert(sh_ord);
   sh_ord->m_qty = order.qty();
   sh_ord->m_side = (strBuy == order.side());
}

//
void OrderCacheImpl::cancelOrder(const std::string& orderId) {
   StrShared ss = make_shared<string>(orderId);
   cancelOrder(ss);
}

//
void OrderCacheImpl::cancelOrder(StrShared orderId) {
   auto orderFound = m_orders.find(orderId);
   if (m_orders.end() == orderFound)
      return; // there is no more order
   OrderImpl* po = orderFound->second.get();
   // remove from user' index
   auto userFound = m_users.find(po->m_user);
   if (m_users.end() == userFound)
      throw "cancelOrder::userNotFound"s;
   auto cnt = userFound->second._orders.erase(orderFound->second);
   if(!cnt)
      throw "cancelOrder::userDoesntOwnSecurity"s;
   // remove from security' index
   auto secFound = m_securs.find(po->m_sec);
   if (m_securs.end() == secFound)
      throw "cancelOrder::securityNotFound"s;
   cnt = secFound->second.erase(orderFound->second);
   if (!cnt)
      throw "cancelOrder::securityIndexInvalid"s;
   m_orders.erase(orderFound);
}

// remove all orders in the cache for this user
void OrderCacheImpl::cancelOrdersForUser(const std::string& user) {
   StrShared su = make_shared<string>(user);
   auto userFound = m_users.find(su);
   if (m_users.end() == userFound)
      return; // there is no such user
   OrderSet& orders  = userFound->second._orders;
   while (!orders.empty()) {
      cancelOrder(orders.begin()->get()->m_id);
   }
}

// remove all orders in the cache for this security with qty >= minQty
void OrderCacheImpl::cancelOrdersForSecIdWithMinimumQty(const std::string& securityId, unsigned int minQty) {
   StrShared ss = make_shared<string>(securityId);
   auto secFound = m_securs.find(ss);
   if (m_securs.end() == secFound)
      return; // no such security
   // first count number of orders
   unsigned count = 0;
   for (auto so : secFound->second) {
      if (so->m_qty >= minQty)
         count++;
   }
   std::vector<StrShared> vs(count);
   for (auto so : secFound->second) {
      if (so->m_qty >= minQty)
         vs.push_back(so->m_id);
   }
   for (auto id : vs) {
      cancelOrder(id);
   }
}





