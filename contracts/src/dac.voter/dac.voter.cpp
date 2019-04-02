#include "../../include/dac/dac.voter.hpp"


namespace eosio {

using std::string;
using std::vector;
using std::to_string;

void voter::handle_transfer( name from, name to, asset quantity, std::string memo ) {
    if (from == _self) return;
    // EOSBet hack
    eosio_assert(to == _self, "transfer must be direct to contract.");
    // Check the buy code is valid
    const string transfer_code = memo.substr(0, 3);
    
    eosio_assert(transfer_code == "vt:" || transfer_code == "", "malformed memo string");
    if (transfer_code == "") { return; }
    
    if (transfer_code == "vt:") {
      handle_vote(from, to, quantity, memo);
    }

   
}

void voter::handle_vote( name from, name to, asset quantity, std::string memo ) {
   // Check the transfer is valid
    eosio_assert(quantity.symbol == symbol("MG", 4), "you must transfer MG tokens for voting");
    eosio_assert( quantity.is_valid(), "quantity is not valid." );

    // Strip buy code from memo
    const string dt = memo.substr(3);
    handle_vote_memo( dt, from, quantity );
}


void voter::handle_vote_memo( string dt, name voter, asset quantity ) {
  /**
   dt format is: vt:1-34222,4-310000, ....
   where 1 - dapp id, 34222 = 3.4222 MG tokens staked for this dapp
  */
  
  voters _voters( _self, _self.value );
  auto itr_voters = _voters.find( voter.value );
  eosio_assert( itr_voters != _voters.end(), "you must unstake your votes before you can vote again" );
  
  vector<vote_row> myvotes;
  
  int tmp_id_length = 0;
  int total_staked = 0;
  
  // current symbol number in dt string
  int i = 0;
  
  char div_id = '-';
  char div_token = ',';
  
  string tmp_id_str = "";
  string tmp_amount_str = "";
  
  while ( i < dt.length() ) {
    
    while ( dt[i] != div_id ) {
      tmp_id_str += dt[i];
      i++;
    }
    i++; // skip the div_id
    
    while ( dt[i] != div_token ) {
      tmp_amount_str += dt[i];
      i++;
    }
    i++; // skip the div_token
    
    // set up the votes 
    
    // Check the guid length is valid
    eosio_assert( tmp_id_str.length() > 0, "malformed dapp id in memo" );

    // todo: here could be the problems cause of pointers?
    const uint64_t dapp_id = std::strtoull(tmp_id_str.c_str(),NULL,0);
    const uint64_t amount = std::strtoull(tmp_amount_str.c_str(),NULL,0);
    total_staked += amount;
    
    vote_row row = vote_row{dapp_id, asset(amount, symbol("MG", 4))};
    myvotes.push_back( row );
    
    
    tmp_id_str = "";
    tmp_amount_str = "";
  
  }
  
  // todo: I need to know the difference like how much it was before the voting
  
  dapps _dapps( _self, _self.value);
  
  
  eosio_assert( total_staked == quantity.amount, 
    string("tranfsfered amount and votes are different: " + to_string(total_staked) + " : " + to_string(quantity.amount)).c_str() );
  // put into the table in here
  
  for(vote_row vt : myvotes) {
    auto itr_dapp = _dapps.find( vt.id ); // by dapp id
    
    _dapps.modify( itr_dapp, same_payer, [&](auto &a) {
      a.dappname = itr_dapp->dappname;
      a.description = itr_dapp->description;
      a.votes = itr_dapp->votes + vt.staked;
    });
    
  }
  
  _voters.emplace( voter, [&](auto &a) {
    a.voter = voter;
    a.votes = myvotes;
    a.total = quantity;
  });
  
  
  
}


void voter::unstake(name account) {
  require_auth( account );
  
  // todo: send the money back
}


void voter::applydapp( name dappname, std::string description ) {
  require_auth( dappname );
  eosio_assert( description.length() < 512, "the description should not be bigger than 512 symbols" );
  
  requests _req( _self, _self.value );
  _req.emplace( dappname, [&](requests_table &a) {
    a.dappname = dappname;
    a.description = description;
  });
}

void voter::acceptdapp( name account, name dappname ) {
  require_auth( account );
  
  dapps _dapps( _self, _self.value );
  requests _requests( _self, _self.value );
  
  auto itr_request = _requests.find( dappname.value );
  eosio_assert( itr_request != _requests.end(), "the requset does not exist" );
  
  const int newid = _next_id();
  
  _dapps.emplace( account, [&](auto &a) {
    a.id = newid;
    a.dappname = dappname;
    a.description = itr_request->description;
    a.votes = asset(0, symbol("MG", 4));
  });
  
  _requests.erase( itr_request );
  
}




uuid voter::_next_id() {
  Config state = _get_config();
  state.last_id = state.last_id + 1;
  eosio_assert(state.last_id > 0, "_next_id overflow detected");
  _update_config(state);
  
  return state.last_id;
};


voter::Config voter::_get_config() {
  Config config;
  
  if (ConfigSingleton.exists()) {
    config = ConfigSingleton.get();
  } else {
    config = Config{};
    ConfigSingleton.set(config, _self);
  }
  
  return config;
}

void voter::_update_config(const Config config) {
//  eosio_assert(has_auth(_self), "Message Error: Only the contract can update config.");
  ConfigSingleton.set(config, _self);
}




// extern "C" {
//   void apply(uint64_t receiver, uint64_t code, uint64_t action) {
//     if (code == name("eosio.token").value && action == name("transfer").value) {
//       execute_action(name(receiver), name(code), &voter::handle_transfer);
//     } 
//     else if (code == receiver && action == name("applydapp").value) {
//       execute_action(name(receiver), name(code), &voter::applydapp);
//     }
//   }

// }

} // eosio namespace

// EOSIO_ABI( eosio::voter, (eosio::applydapp) )

EOSIO_DISPATCH(eosio::voter, (eosio::applydapp))


