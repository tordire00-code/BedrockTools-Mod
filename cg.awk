/^\/\/ ===== /{fn=$3; sz[fn]=0; next}
fn!=""{sz[fn]++; l=$0; while(match(l,/FUN_[0-9a-fA-F]+/)){c=substr(l,RSTART,RLENGTH); indeg[c]++; if(!((fn,c) in edge)){edge[fn,c]=1; outdeg[fn]++} l=substr(l,RSTART+RLENGTH)}}
END{for(f in sz)print "SIZE\t"sz[f]"\t"f; for(f in indeg)print "INDEG\t"indeg[f]"\t"f; for(f in outdeg)print "OUTDEG\t"outdeg[f]"\t"f; print "NFUNC\t"length(sz)"\t-"}
