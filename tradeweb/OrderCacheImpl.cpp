#include <list>

#include "OrderCacheImpl.h"

using std::string;
using std::make_shared;
using namespace std::string_literals;

static const std::string strBuy{"Buy"s};
static const std::string strSell{"Sell"s};

//
void OrderCacheImpl::addOrder(Order order) {
   StrShared id = make_shared<string>(order.orderId());
   auto orderRes = m_orders.insert({id, OrderShared{}});
   if (!orderRes.second)
      return; // already exist
   OrderShared sh_ord = make_shared<OrderImpl>();
   orderRes.first->second = sh_ord;
   sh_ord->m_id = id;
   // assign user
   StrShared user = make_shared<string>(order.user());
   auto userRes = m_users.insert({user, User{}});
   auto userIter = userRes.first;
   if (userRes.second) { // new user
      // have to search for the company
      StrShared comp = make_shared<string>(order.company());
      auto compRes = m_companies.insert(comp);
      userIter->second._comp = *(compRes.first);
   }
   sh_ord->m_user = userIter->first;
   sh_ord->m_comp = userIter->second._comp;
   userIter->second._orders.insert(sh_ord);
   // assign security
   StrShared sec = make_shared<string>(order.securityId());
   auto compRes = m_securs.insert({sec, OrderSet{}});
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
   // first gather all ids
   std::list<StrShared> lo;
   for (auto so : secFound->second) {
      if (so->m_qty >= minQty)
         lo.push_back(so->m_id);
   }
   for (auto id : lo) {
      cancelOrder(id);
   }
}

// return the total qty that can match for the security id
unsigned  OrderCacheImpl::getMatchingSizeForSecurity(const std::string& securityId) {
   StrShared ss = make_shared<string>(securityId);
   auto secFound = m_securs.find(ss);
   if (m_securs.end() == secFound)
      return 0; // no such security
   CompQtyList buy;
   CompQtyList sell;
   unsigned total = 0;
   // for each order find match, what's not matched, add to list
   for (auto so : secFound->second) {
      unsigned qtyRest = so->m_qty;
      CompQtyList& otherSide = so->m_side ? sell : buy;
      for (auto iter = otherSide.begin(); (otherSide.end() != iter) && (qtyRest > 0); ) {
         if (iter->_comp != so->m_comp) {
            unsigned qtyCurr = iter->_qty;
            unsigned matched = qtyCurr > qtyRest ? qtyRest : qtyCurr;
            qtyRest -= matched;
            total += matched;
            iter->_qty -= matched;
            if (!iter->_qty)
               iter = otherSide.erase(iter);
            else
               ++iter;
         }
         else {
            ++iter;
         }
      }
      if (qtyRest) { // still have unmatched qty - add it to same company
         CompQtyList& sameSide = so->m_side ? buy : sell;
         auto iterComp = std::find_if(sameSide.begin(), sameSide.end(), [&so](CompQty& cq) { return so->m_comp == cq._comp; });
         if (sameSide.end() == iterComp)
            sameSide.push_front({so->m_comp, qtyRest});
         else
            iterComp->_qty += qtyRest;
      }
   }
   return total;
}

//
std::vector<Order> OrderCacheImpl::getAllOrders() const {
   std::vector<Order> orders;
   for (auto po : m_orders) {
      OrderImpl* oi = po.second.get();
      Order ord(*(oi->m_id), *(oi->m_sec), (oi->m_side ? strBuy : strSell), oi->m_qty, *(oi->m_user), *(oi->m_comp));
      orders.emplace_back(std::move(ord));
   }
   return orders;
}



