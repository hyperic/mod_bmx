# Invoke as awk addloadexample.awk

BEGIN {
  lms = 0;
} 

tolower($0) ~ /^[# \t]*loadmodule[ \t]/ {
  if ( $2 == MODULE "_module" ) {
    print "LoadModule " MODULE "_module " LIBPATH "/mod_" MODULE DSO;
    lms = 2;
    next;
  }
  # test $3 since # LoadModule is split into two tokens
  else if ( $3 == MODULE "_module" ) {
    print $1 "LoadModule " MODULE "_module " LIBPATH "/mod_" MODULE DSO;
    lms = 2;
    next;
  }
  else if ( ! lms ) lms = 1;
}

$0 ~ /^[ \t]*$/ && lms == 1 {
  print "LoadModule " MODULE "_module " LIBPATH "/mod_" MODULE DSO;
  lms = 2;
} 

tolower($0) ~ /^[# \t]*include[ \t]/ && $NF == EXAMPLECONF {
  lms = 3;
}

{ print }

END {
  if ( lms < 3 ) { 
    if ( $0 !~ /^[ \t]*$/ ) print "";
    if ( lms < 2 ) { 
      print "LoadModule " MODULE "_module " LIBPATH "/mod_" MODULE DSO;
      print "";
    }
    if ( length(EXAMPLECONF) ) {
      print "# Example mod_" MODULE " configuration";
      print "#Include " EXAMPLECONF "\n";
    }
  }
}

