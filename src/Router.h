/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2010 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the declaration of the router
 */

#ifndef __NOXIMROUTER_H__
#define __NOXIMROUTER_H__

#include <systemc.h>
#include <bitset>
#include "DataStructs.h"
#include "Buffer.h"
#include "Stats.h"
#include "GlobalRoutingTable.h"
#include "LocalRoutingTable.h"
#include "ReservationTable.h"
#include "Utils.h"
#include "routingAlgorithms/RoutingAlgorithm.h"
#include "routingAlgorithms/RoutingAlgorithms.h"
#include "selectionStrategies/SelectionStrategy.h"
#include "selectionStrategies/SelectionStrategies.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

using namespace std;

extern unsigned int drained_volume;

SC_MODULE(Router)
{

    // I/O Ports
    sc_in_clk clock;		                  // The input clock for the router
    sc_in <bool> reset;                           // The reset signal for the router

    // number of ports: 4 mesh directions + local + wireless 
    sc_in <Flit> flit_rx[DIRECTIONS + 2];	  // The input channels 
    sc_in <bool> req_rx[DIRECTIONS + 2];	  // The requests associated with the input channels
    sc_out <bool> ack_rx[DIRECTIONS + 2];	  // The outgoing ack signals associated with the input channels

    sc_out <Flit> flit_tx[DIRECTIONS + 2];   // The output channels
    sc_out <bool> req_tx[DIRECTIONS + 2];	  // The requests associated with the output channels
    sc_in <bool> ack_tx[DIRECTIONS + 2];	  // The outgoing ack signals associated with the output channels

    sc_out <int> free_slots[DIRECTIONS + 1];
    sc_in <int> free_slots_neighbor[DIRECTIONS + 1];

    // Neighbor-on-Path related I/O
    sc_out < NoP_data > NoP_data_out[DIRECTIONS];
    sc_in < NoP_data > NoP_data_in[DIRECTIONS];

    // Registers

    /*
       Coord position;                     // Router position inside the mesh
     */
    int local_id;		                // Unique ID
    int routing_type;		                // Type of routing algorithm
    int selection_type;
    Buffer buffer[DIRECTIONS + 2];	        // Buffer for each input channel 
    bool current_level_rx[DIRECTIONS + 2];	// Current level for Alternating Bit Protocol (ABP)
    bool current_level_tx[DIRECTIONS + 2];	// Current level for Alternating Bit Protocol (ABP)
    Stats stats;		                // Statistics
    Power power;
    LocalRoutingTable routing_table;	// Routing table
    ReservationTable reservation_table;	// Switch reservation table
    int start_from_port;	                // Port from which to start the reservation cycle
    unsigned long routed_flits;
    RoutingAlgorithm * routingAlgorithm; 
    SelectionStrategy * selectionStrategy; 
    
    sc_uint<MAX_FLIT_PAYLOAD> previous_payload[DIRECTIONS + 2]; //23/06/15 Erwan Memorize previous payload to compute the number
                                                                //of transitions between concurrent flits
    int transition_types[22];   //08/07/15, array that memorizes all occurrences of each transitions between 2 words
    double current_alpha;       //Erwan, value to get instant value of switching activity
    ofstream streamAlphaFile;
    string alphaFileName;       //File name specific to a PE for application traffic
    
    
    // Functions

    void rxProcess();		// The receiving process
    void txProcess();		// The transmitting process
    void perCycleUpdate();
    void configure(const int _id, const double _warm_up_time,
		   const unsigned int _max_buffer_size,
		   GlobalRoutingTable & grt);

    unsigned long getRoutedFlits();	// Returns the number of routed flits 
    unsigned int getFlitsCount();	// Returns the number of flits into the router

    // Constructor

    SC_CTOR(Router) {
	SC_METHOD(rxProcess);
	sensitive << reset;
	sensitive << clock.pos();

	SC_METHOD(txProcess);
	sensitive << reset;
	sensitive << clock.pos();

	SC_METHOD(perCycleUpdate);
	sensitive << reset;
	sensitive << clock.pos();
	
    routingAlgorithm = RoutingAlgorithms::get(GlobalParams::routing_algorithm);

    if (routingAlgorithm == 0)
        assert(false);

    selectionStrategy = SelectionStrategies::get(GlobalParams::selection_strategy);

    if (selectionStrategy == 0)
        assert(false);
    }

  private:

    // performs actual routing + selection
    int route(const RouteData & route_data);

    // wrappers
    int selectionFunction(const vector <int> &directions,
			  const RouteData & route_data);
    vector < int >routingFunction(const RouteData & route_data);
 
    NoP_data getCurrentNoPData();
    void NoP_report() const;
    int NoPScore(const NoP_data & nop_data, const vector <int> & nop_channels) const;
    int reflexDirection(int direction) const;
    int getNeighborId(int _id, int direction) const;

  public:
    unsigned int local_drained;

    bool inCongestion();
    void ShowBuffersStats(std::ostream & out);
        
    void compute_nb_transitions(sc_uint<MAX_FLIT_PAYLOAD> cur_word,sc_uint<MAX_FLIT_PAYLOAD> previous_word, int array_transition[]);
    //check the number of transitions to compute after link power consumption
    
};

#endif
