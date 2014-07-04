#!/bin/bash

##
## Setup functions
##
function show_time () 
{
    num=$1
    min=0
    hour=0
    day=0
    if((num>59));then
        ((sec=num%60))
        ((num=num/60))
        if((num>59));then
            ((min=num%60))
            ((num=num/60))
            if((num>23));then
                ((hour=num%24))
                ((day=num/24))
            else
                ((hour=num))
            fi
        else
            ((min=num))
        fi
    else
        ((sec=num))
    fi
    echo "$day"d "$hour"h "$min"m "$sec"s
}
date2stamp () 
{
    date --utc --date "$1" +%s
}
stamp2date ()
{
    date --utc --date "1970-01-01 $1 sec" "+%d-%m-%Y %T"
}

dateDiff ()
{
    case $1 in
        -s)   sec=1;      shift;;
        -m)   sec=60;     shift;;
        -h)   sec=3600;   shift;;
        -d)   sec=86400;  shift;;
        *)    sec=86400;;
    esac
    dte1=$(date2stamp $1)
    dte2=$(date2stamp $2)
    diffSec=$((dte2-dte1))
    if ((diffSec < 0)); then abs=-1; else abs=1; fi
    echo $((diffSec/sec*abs))
}

reset_session()
{
	##
	## Setup session
	##
	declare -i STARTSTAMP=$(date "+%s")
	declare -i ENDSTAMP=0
	local THISENDSTAMP=$(( STARTSTAMP + run_interval ))
	echo "$THISENDSTAMP"
}
create_log_file()
{
	log_file_date=$(date "+%d-%m-%Y %T")
	log_file="$path_to_logfile_dir/ccMinerLog-$log_file_date.log"
	error_log_file="$path_to_logfile_dir/ccMinerLog-$log_file_date.log.error"
}

log_to_file()
{
	##
	## Set default log file
	##
	## $1=text to output
	## $2=output to terminal (null, boolean)
	## $3=log type (null, log, error)
	## $4=force to terminal (null, boolean)
	local this_log_file=$log_file 
	current_time_string=$(date "+%d-%m-%Y %T")
	##
	## What type of log is this?
	##
	if [ "$3" = "error" ]; 
	then
		this_log_file=$error_log_file
	fi
	##
	## Write the log to file
	##
	if [ $logging_enabled = true ] || [ "$3" = "error" ];
	then
		echo -e "$current_time_string :: $1" >> "$this_log_file" 2>&1
	fi
	##
	## Output to terminal?
	##
	if ([ "$2" = true ] && [ $show_terminal = true ]) || ([ "$4" = true ]) || ([ "$3" = "error" ]); 
	then
		if [ "$3" = "error" ];
		then		
			echo -e "\033[1;37;41m$current_time_string :: $1\033[0m"
		else
			echo -e "$current_time_string :: $1"
		fi
	fi
}

start_ccminer()
{
	##
	## $1=failover status (boolean)
	##
	log_to_file "$lang_starting_ccminer..." true null true
	##
	## Make sure all GPU's specified are available
	##
	
	##
	## Check that Our Hero isn't working already
	##
	if ps aux | grep "[c]cminer" > /dev/null
	then
	    log_to_file "$lang_start_aborted..."  true null true
	else
		if [ "$1" = true ] && [ $failover = false ];
		then
			gnome-terminal -x $path_to_ccminer_dir/ccminer -a $fo_pool_algo -o $fo_pool_url -u $fo_pool_user -p $fo_pool_password -d $pool_gpus $fo_pool_flags
			failover=true
		else
	    	gnome-terminal -x $path_to_ccminer_dir/ccminer -a $pool_algo -o $pool_url -u $pool_user -p $pool_password -d $pool_gpus $pool_flags
	    	failover=false
	    fi
	fi
} 

stop_ccminer()
{
	log_to_file "$lang_stopping_ccminer..." true
	pkill ccminer
}

process_email()
{
	## $1 = email text
	## $2 = email subject
	
	if [ $send_email = true ];
	then
		echo -e "$1" | mail -s "$2" $user_email
	fi
}

