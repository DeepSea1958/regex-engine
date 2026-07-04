digraph DFA {
    rankdir=LR;
    node [shape=circle];

    start [shape=point];
    S0 [shape=circle, label="0"];
    S1 [shape=doublecircle, label="1"];
    S2 [shape=doublecircle, label="2"];

    /* start edge */
    start -> S0;

    /* transitions (character ranges merged) */
    S0 -> S2 [label="a"];
    S0 -> S1 [label="b"];
    S1 -> S1 [label="c"];
}
