include "globals.mzn";

% Input Parameters (values for these specified in .dzn data file)
int: NUM_NODES; 
int: NUM_EDGES;
int: MAX_COLOR = 85;
array[1..NUM_EDGES,1..2] of int: edges;

int: NUM_CLIQUE_NODES;
array[1..NUM_CLIQUE_NODES] of 0..NUM_NODES-1: clique;

% Decision Variable: the solver will find the values of these
array[0..NUM_NODES-1] of var 1..MAX_COLOR: color;

% Our Constraints
% symmetry breaking
constraint forall(i in 1..NUM_CLIQUE_NODES)(color[clique[i]] = i);
constraint forall(n in NUM_CLIQUE_NODES+1..MAX_COLOR-1)(value_precede(n, n+1, color));

constraint forall(e in 1..NUM_EDGES)(color[edges[e,1]] != color[edges[e,2]]);

% first_fail  - nodes=1218954
% smallest    - fail
% dom_w_deg   - nodes=204149
% input_order - nodes=7146149

% dom_w_deg + indomain_split - nodes=60070
solve :: int_search(color, dom_w_deg, indomain_split)
 satisfy;

% formatted output
output[ show(max(color)), show(color) ];