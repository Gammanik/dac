#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>

namespace eosiosystem {
class system_contract;
}


namespace eosio {
  
  typedef uint32_t unix_time;
  typedef uint64_t uuid;


class [[eosio::contract("voter")]] voter : public contract {
public:
  // using contract::contract;
  
  voter(name self, name code, datastream<const char *> ds) : eosio::contract(self, code, ds),
    ConfigSingleton(_self, _self.value) { }
  
  void handle_transfer(name from, name to, asset quantity, std::string memo);
  
  [[eosio::action]]
  void applydapp( name dappname, std::string description );
  
  [[eosio::action]]
  void acceptdapp( name account, name dappname );
  
  [[eosio::action]]
  void unstake( name account );
  
private:
  void handle_vote(name from, name to, asset quantity, std::string memo);
  void handle_vote_memo(std::string memo, name voter, asset quantity );
  
  struct [[eosio::table]] requests_table {
    name dappname;
    std::string description;
    std::string response; // 'waiting' || 'rejected' ?
    
    uint64_t primary_key()const { return dappname.value; }
  };
  
  
  struct [[eosio::table]] dapps_table {
    uuid  id;
    name      dappname;
    std::string    description;
    asset     votes;
    
    uint64_t primary_key()const { return id; }
  };
  
  
  struct vote_row { uuid id;  asset staked; };
  struct [[eosio::table]] voters_table {
    name    voter;
    std::vector<vote_row> votes; // = []; // the dist of votes
    
    asset   total;
    unix_time stamp;
    
    uint64_t primary_key()const { return voter.value; }
  };
  
  typedef eosio::multi_index< "dapps"_n, dapps_table > dapps;
  typedef eosio::multi_index< "voters"_n, voters_table > voters;
  typedef eosio::multi_index< "requests"_n, requests_table > requests;

  
  
  
  struct [[eosio::table("settings")]] Config {
    uuid last_id = 1;
    // time when the last drop was held
    unix_time last_drop_time =  0; // todo: 3*24*3600
    // time of the next drop
    unix_time next_drop_time = 0; // todo: 3*24*3600 + 1980 year
  };

  typedef singleton<name("settings"), Config> SingletonType;
  SingletonType ConfigSingleton;
  
  Config _get_config();
  void _update_config(const voter::Config config);
  uuid _next_id();
  
};


}


// EOSIO_ABI(voter, ());