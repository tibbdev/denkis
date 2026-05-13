
#include <zenoh.hpp>

int main() 
{
    auto session = zenoh::expect(zenoh::open(zenoh::Config::create_default()));
    
    // Listen for raw data to drive state machines
    auto sub = session.declare_subscriber("denkis/serial/rx", [](const zenoh::Sample& s){
        // Process data for state machine transitions
    });

    // Provide a "Queryable" so the GUI can ask for current test status
    auto queryable = session.declare_queryable("denkis/logic/status", [](const zenoh::Query& q){
        q.reply(zenoh::Sample("denkis/logic/status", "TEST_IN_PROGRESS"));
    });

    while(true) { /* Run State Machine */ }
}