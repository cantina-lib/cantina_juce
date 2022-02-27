import("stdfaust.lib");
rx = library("lib/routing_ext.lib");

// MUST BE A POWER OF TWO!!
N = 8;//nentry("N", 4, 4, 4, 4);
k = 2;
// process = an.rtorv(N) : rvwcf(N, k);
process = rx.argMax(N);
//process = swap(N, 4,8);