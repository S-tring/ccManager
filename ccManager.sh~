#!/bin/bash

##
## Setup
##
declare -r ccManagerDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
## Sources
source $ccManagerDIR/config/config.conf
source $ccManagerDIR/pools/primary.pool
source $ccManagerDIR/pools/secondary.pool
source $ccManagerDIR/config/constants.conf
source $ccManagerDIR/language/$system_lang.lang
source $ccManagerDIR/includes/functions.inc
source $ccManagerDIR/includes/monitor.inc       
##
## Create the log file
##
echo "$lang_creating_logfile_paths"
create_log_file
##
## Start ccMiner
##
ENDSTAMP=$(reset_session)
start_ccminer
session_start=$(date "+%s")
##
## Start monitoring
##
while [ $lost_gpu -lt 2 ]
do
	##
	## Get current time
	##
	declare -i CURRENTTIMESTAMP=$(date "+%s")
	##
	## Output some info
	##
	echo -e "\033[1;37;42mccManager $lang_author_details\033[0m"
	log_to_file "$output_divider" true "log" true

	if [ $failover = false ];
	then
		current_pool_url=$pool_url
	else
		current_pool_url=$fo_pool_url
	fi

	log_to_file "$current_pool_url" true null true
	
	run_time=$((CURRENTTIMESTAMP-session_start))
	run_time=$(show_time $run_time)
	total_runtime=$((CURRENTTIMESTAMP-original_starttime))
	total_runtime=$(show_time $total_runtime)
	outstanding_time=$((ENDSTAMP-CURRENTTIMESTAMP))
	outstanding_time=$(show_time $outstanding_time) 
	
	log_to_file "$lang_session_runtime: $run_time" true null true
	log_to_file "$lang_session_ends: $outstanding_time" true null true
	log_to_file "$lang_total_runtime: $total_runtime" true null true
	log_to_file "$output_divider" true null true
	##
	## Grab GPU details
	##
	get_gpu_details
	##
	## Wait time between next run
	##
	echo -e "\033[30;42m$lang_press_to_break ...\033[0m"
	sleep $wait_time
	##
	## Have we finished this loop?
	##
	if [ $CURRENTTIMESTAMP -gt $ENDSTAMP ] && [ $run_interval -gt 0 ]; 
	then	
		##
		## Stop ccMiner and Output to terminal
		##
		stop_ccminer

		log_to_file "$lang_session_ended_waiting $wait_time $lang_seconds..." true null true
		sleep $wait_time
		##
		## Restart ccMiner
		##
		ENDSTAMP=$(reset_session)
		failover=false
		start_ccminer
		session_start=$(date "+%s")
		##
		## Send confirmation by email?
		##

	fi

done
##
## If we reach here, we must stop mining
##
## Reset single card low temp here?
stop_ccminer
	
if [ $lost_gpu -gt 1 ];
then	
	log_to_file "$lang_error_halted_unfixable" true "error"
	process_email "$lang_error_halted_unfixable" "$lang_error_critical_alert"
fi


echo "$lang_finish"
