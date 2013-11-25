#include <gtest/gtest.h>

#include <string>
#include <set>
#include <vector>

#include "bid.h"
#include "bid_portfolio.h"
#include "mock_facility.h"
#include "request.h"
#include "request_portfolio.h"
#include "resource.h"
#include "resource_helpers.h"
#include "test_context.h"
#include "trader.h"

#include "exchange_context.h"

using std::set;
using std::string;
using std::vector;

using cyclus::Bid;
using cyclus::BidPortfolio;
using cyclus::ExchangeContext;
using cyclus::PrefMap;
using cyclus::Request;
using cyclus::RequestPortfolio;
using cyclus::Resource;
using cyclus::TestContext;
using cyclus::Trader;
using test_helpers::get_mat;
using test_helpers::get_req;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class ExchangeContextTests: public ::testing::Test {
 protected:
  TestContext tc;
  MockFacility* fac1;
  MockFacility* fac2;
  Request<Resource>::Ptr req1, req2;
  RequestPortfolio<Resource>::Ptr rp1, rp2;
  string commod1, commod2;
  double pref;
  
  virtual void SetUp() {
    fac1 = new MockFacility(tc.get());
    fac2 = new MockFacility(tc.get());

    pref = 0.5;
    commod1 = "commod1";
    req1 = Request<Resource>::Ptr(
        new Request<Resource>(get_mat(), fac1, commod1, pref));
    
    req2 = Request<Resource>::Ptr(
        new Request<Resource>(get_mat(), fac2, commod1, pref));

    rp1 = RequestPortfolio<Resource>::Ptr(new RequestPortfolio<Resource>());
    rp1->AddRequest(req1);    
    rp2 = RequestPortfolio<Resource>::Ptr(new RequestPortfolio<Resource>());
    rp2->AddRequest(req2);
  };
  
  virtual void TearDown() {
    delete fac1;
    delete fac2;
  };
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(ExchangeContextTests, Empty) {
  ExchangeContext<Resource> context;
  EXPECT_TRUE(context.requests().empty());
  EXPECT_TRUE(context.RequestsForCommod(commod2).empty());
  EXPECT_TRUE(context.RequestsForCommod(commod2).empty());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(ExchangeContextTests, AddRequest1) {
  // 1 request for 1 commod
  ExchangeContext<Resource> context;
    
  context.AddRequestPortfolio(rp1);

  std::vector<RequestPortfolio<Resource>::Ptr> vp;
  vp.push_back(rp1);
  EXPECT_EQ(vp, context.requests());
  
  EXPECT_EQ(1, context.RequestsForCommod(commod1).size());  
  std::vector<Request<Resource>::Ptr> vr;
  vr.push_back(req1);
  EXPECT_EQ(vr, context.RequestsForCommod(commod1));

  EXPECT_EQ(1, context.requesters().size());  
  std::set<Trader*> requesters;
  requesters.insert(fac1);
  EXPECT_EQ(requesters, context.requesters());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(ExchangeContextTests, AddRequest2) {
  // 2 requests for 1 commod
  ExchangeContext<Resource> context;
    
  context.AddRequestPortfolio(rp1);
  context.AddRequestPortfolio(rp2);

  std::vector<RequestPortfolio<Resource>::Ptr> vp;
  vp.push_back(rp1);
  vp.push_back(rp2);
  EXPECT_EQ(vp, context.requests());
  
  EXPECT_EQ(2, context.RequestsForCommod(commod1).size());  
  std::vector<Request<Resource>::Ptr> vr;
  vr.push_back(req1);
  vr.push_back(req2);
  EXPECT_EQ(vr, context.RequestsForCommod(commod1));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(ExchangeContextTests, AddRequest3) {
  // 2 requests for 2 commod
  Request<Resource>::Ptr req(new Request<Resource>(get_mat(), fac1, commod2));
  rp1->AddRequest(req);
  
  ExchangeContext<Resource> context;
    
  context.AddRequestPortfolio(rp1);
  
  EXPECT_EQ(1, context.RequestsForCommod(commod1).size());
  EXPECT_EQ(1, context.RequestsForCommod(commod2).size());
  
  std::vector<Request<Resource>::Ptr> vr;
  vr.push_back(req1);
  EXPECT_EQ(vr, context.RequestsForCommod(commod1));

  vr.clear();
  vr.push_back(req);
  EXPECT_EQ(vr, context.RequestsForCommod(commod2));  
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(ExchangeContextTests, AddBid1) {
  // bid bid for a request
  ExchangeContext<Resource> context;
  context.AddRequestPortfolio(rp1);

  EXPECT_TRUE(context.BidsForRequest(req1).empty());
  
  Bid<Resource>::Ptr bid(new Bid<Resource>(req1, get_mat(), fac1));
  BidPortfolio<Resource>::Ptr bp1(new BidPortfolio<Resource>());
  bp1->AddBid(bid);

  context.AddBidPortfolio(bp1);

  std::vector<BidPortfolio<Resource>::Ptr> vp;
  vp.push_back(bp1);
  EXPECT_EQ(vp, context.bids());

  EXPECT_EQ(1, context.BidsForRequest(req1).size());
  
  std::vector<Bid<Resource>::Ptr> vr;
  vr.push_back(bid);
  EXPECT_EQ(vr, context.BidsForRequest(req1));

  EXPECT_EQ(1, context.bidders().size());  
  std::set<Trader*> bidders;
  bidders.insert(fac1);
  EXPECT_EQ(bidders, context.bidders());

  PrefMap<Resource>::type obs;
  obs[req1].push_back(std::make_pair(bid, req1->preference()));
  EXPECT_EQ(context.Prefs(req1->requester()), obs);
  obs.clear();
  obs[req1].push_back(std::make_pair(bid, req1->preference() * 0.1));
  EXPECT_NE(context.Prefs(req1->requester()), obs);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TEST_F(ExchangeContextTests, AddBid2) {
  // multiple bids for multiple requests
  ExchangeContext<Resource> context;
  context.AddRequestPortfolio(rp1);
  context.AddRequestPortfolio(rp2);

  EXPECT_TRUE(context.BidsForRequest(req1).empty());
  EXPECT_TRUE(context.BidsForRequest(req2).empty());

  // bid1 and bid2 are from one bidder (fac1)
  BidPortfolio<Resource>::Ptr bp1(new BidPortfolio<Resource>());
  Bid<Resource>::Ptr bid1(new Bid<Resource>(req1, get_mat(), fac1));
  bp1->AddBid(bid1);
  Bid<Resource>::Ptr bid2(new Bid<Resource>(req2, get_mat(), fac1));
  bp1->AddBid(bid2);
  
  // bid3 and bid4 are from one bidder (fac2)
  BidPortfolio<Resource>::Ptr bp2(new BidPortfolio<Resource>());
  Bid<Resource>::Ptr bid3(new Bid<Resource>(req1, get_mat(), fac2));
  bp2->AddBid(bid3);
  Bid<Resource>::Ptr bid4(new Bid<Resource>(req2, get_mat(), fac2));
  bp2->AddBid(bid4);

  std::vector<BidPortfolio<Resource>::Ptr> vp;
  std::vector<Bid<Resource>::Ptr> vreq1;
  std::vector<Bid<Resource>::Ptr> vreq2;

  // add bids from first bidder
  context.AddBidPortfolio(bp1);

  vp.push_back(bp1);
  EXPECT_EQ(vp, context.bids());
  
  vreq1.push_back(bid1);
  vreq2.push_back(bid2);
  EXPECT_EQ(1, context.BidsForRequest(req1).size());
  EXPECT_EQ(1, context.BidsForRequest(req2).size());
  EXPECT_EQ(vreq1, context.BidsForRequest(req1));
  EXPECT_EQ(vreq2, context.BidsForRequest(req2));
  
  // add bids from second bidder
  context.AddBidPortfolio(bp2);

  vp.push_back(bp2);
  EXPECT_EQ(vp, context.bids());
  
  vreq1.push_back(bid3);
  vreq2.push_back(bid4);
  EXPECT_EQ(2, context.BidsForRequest(req1).size());
  EXPECT_EQ(2, context.BidsForRequest(req2).size());
  EXPECT_EQ(vreq1, context.BidsForRequest(req1));
  EXPECT_EQ(vreq2, context.BidsForRequest(req2));

  EXPECT_EQ(2, context.bidders().size());  
  std::set<Trader*> bidders;
  bidders.insert(fac1);
  bidders.insert(fac2);
  EXPECT_EQ(bidders, context.bidders());
}
