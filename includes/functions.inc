#!/bin/bash

##################################################################
##################################################################
##
##	ccManager v0.0.3-alpha (10/07/2014) - "Beta in sight..."
##	- String
##
##################################################################
##################################################################

##
## Setup functions
##
package_installed()
{
	## ${1} - package name
	local installed=false
	
	dpkg-query -Wf'${db:Status-abbrev}' "${1}" 2>/dev/null | grep -q '^i'
	
	if dpkg-query -Wf'${db:Status-abbrev}' "${1}" 2>/dev/null | grep -q '^i'; 
	then
	    installed=true
	fi
	
	echo $installed
}
show_time () 
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
		error_count=$((error_count+1))
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

find_ccminer()
{
	##$1=path_to_ccMiner
	##$2=error language var
	##$3=success language var
	
	if [ ! -f $1/ccminer ];
	then
		log_to_file "$2" true "error"
		exit
	else
		log_to_file "$3..." true null true
	fi
}

start_ccminer()
{
	##
	## $1=failover status (boolean)
	##
	ENDSTAMP=$(reset_session)
	session_start=$(date "+%s")
	log_to_file "$lang_starting_ccminer..." true null true
	##
	## Make sure all GPU's specified are available
	##
	find_gpus
	##
	## Check that Our Hero isn't working already
	##
	if ps aux | grep "[c]cminer" > /dev/null
	then
	    log_to_file "$lang_start_aborted..."  true null true
	else
		if [ "$1" = true ] && [ $failover = false ];
		then
			find_ccminer $fo_pool_path_to_ccminer_dir "$lang_error_secondary_ccminer_not_found" "$lang_secondary_ccminer_allocated"
			current_miner=$fo_pool_path_to_ccminer_dir/ccminer
			gnome-terminal -x $current_miner -a $fo_pool_algo -o $fo_pool_url -u $fo_pool_user -p $fo_pool_password -d $pool_gpus $fo_pool_flags
			failover=true
		else
			find_ccminer $pool_path_to_ccminer_dir "$lang_error_primary_ccminer_not_found" "$lang_primary_ccminer_allocated"
			current_miner=$pool_path_to_ccminer_dir/ccminer
	    	gnome-terminal -x $current_miner -a $pool_algo -o $pool_url -u $pool_user -p $pool_password -d $pool_gpus $pool_flags
	    	failover=false
	    fi
	fi
} 

stop_ccminer()
{
	log_to_file "$lang_stopping_ccminer..." true
	pkill ccminer
	current_miner=""
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

invoke_failover()
{
	failover_count=$((failover_count+1))
	stop_ccminer
	log_to_file "$lang_error_failover_activated..." true "error"
	##
	## Need to send email here
	##
	process_email "$lang_error_ccminer_failover_activated" "$lang_error_alert"
	sleep 5
}

find_gpus()
{
	if [ ${#used_gpus[@]} -lt 1 ];
	then
		log_to_file "$lang_error_no_gpus $lang_error_check_config_settings" true "error"
		exit
	fi
	for i in "${!used_gpus[@]}"
	do 
		if ! [ "${used_gpus[$i]}" -eq "${used_gpus[$i]}" >& /dev/null ];
		then
			log_to_file "$lang_error_invalid_gpu${used_gpus[$i]}. $lang_error_check_config_settings" true "error"
			exit
		else
			##
			## We got a numeric GPU, but is it valid?
			## ... let's take the patient's temperature :-)
			##
			thermostat=$(nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader,nounits --id=${used_gpus[$i]})
			if ! [ "${thermostat}" -eq "${thermostat}" >& /dev/null ];
			then
				log_to_file "GPU #${used_gpus[$i]} $lang_error_does_not_exist $lang_error_check_config_settings" true "error"
				exit
			fi
		fi
	done 
	log_to_file "$lang_gpus_available..." true null true
}
