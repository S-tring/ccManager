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
## Set some vars
##
declare -r ccManagerDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source $ccManagerDIR/config/constants.conf

declare -i lost_gpu=0
declare -i lowtemp_time=0
declare -i failover_count=0
declare -i poolswitch_count=0
declare current_miner=""
declare error_count=0
declare failover=false

## Additional sources
source $ccManagerDIR/config/config.conf
source $ccManagerDIR/pools/primary.pool
source $ccManagerDIR/pools/secondary.pool
source $ccManagerDIR/language/$system_lang.lang
source $ccManagerDIR/includes/functions.inc
source $ccManagerDIR/includes/monitor.inc 

IFS=', ' read -a used_gpus <<< "$pool_gpus"
##
## Create the log file
##
echo "$lang_creating_logfile_paths..."
create_log_file
##
## Do initial validation scans...
##
## Do we require email facilities?
##
if [ $send_email = true ];
then
	mailutils_installed=$(package_installed "mailutils")
	if [ $mailutils_installed = true ];
	then
		log_to_file "$lang_mailutils_success..." true null true
	else
		while true;
		do
			read -p "$lang_error_mailutils" yn
			case $yn in
				[Yy]* ) echo "$lang_continuing..."; break;;
				[Nn]* ) echo "$lang_exiting..."; exit;;
				* ) echo "$lang_yes_or_no";;
			esac
		done
	fi
fi 
##
## Are our gpu's available?
find_gpus
##
## Is ccMiner available?
##

## Set for primary pool
if [ $pool_path_to_ccminer_dir = false ];
then
	pool_path_to_ccminer_dir=$path_to_ccminer_dir
fi
## Set for secondary pool
if [ $fo_pool_path_to_ccminer_dir = false ];
then
	fo_pool_path_to_ccminer_dir=$path_to_ccminer_dir
fi
find_ccminer $path_to_ccminer_dir "$lang_error_default_ccminer_not_found" "$lang_default_ccminer_available"
find_ccminer $pool_path_to_ccminer_dir "$lang_error_primary_ccminer_not_found" "$lang_primary_ccminer_available"
find_ccminer $fo_pool_path_to_ccminer_dir "$lang_error_secondary_ccminer_not_found" "$lang_secondary_ccminer_available"

## 
## Validation checks complete...
## ...Start ccMiner
##
##ENDSTAMP=$(reset_session)
start_ccminer
##session_start=$(date "+%s")
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
	echo -e "\033[1;37;42mccManager $lang_ccManager_ver $lang_author_details\033[0m"
	log_to_file "$output_divider" true "log" true

	if [ ${#current_miner} -gt 0 ];
	then
		log_to_file "$lang_miner: $current_miner" true null true
	fi
	
	if [ $failover = false ];
	then
		current_pool_url=$pool_url
	else
		current_pool_url=$fo_pool_url
	fi

	log_to_file "$lang_pool: $current_pool_url" true null true
	
	run_time=$((CURRENTTIMESTAMP-session_start))
	run_time=$(show_time $run_time)
	total_runtime=$((CURRENTTIMESTAMP-original_starttime))
	total_runtime=$(show_time $total_runtime)
	outstanding_time=$((ENDSTAMP-CURRENTTIMESTAMP))
	outstanding_time=$(show_time $outstanding_time) 
	
	log_to_file "$lang_session_runtime: $run_time" true null true
	log_to_file "$lang_session_ends: $outstanding_time" true null true
	log_to_file "$lang_total_runtime: $total_runtime" true null true
	##
	## Show pool switches?
	##
	if [ $switch_pools = true ];
	then
		log_to_file "\033[1;33m$lang_pool_switches: $poolswitch_count\033[0m" true null true
	fi
	##
	## Show Failovers ??
	##
	if [ $failover_count -gt 0 ];
	then
		log_to_file "\033[1;33m$lang_failovers: $failover_count\033[0m" true null true
	fi
	##
	## Show error count?
	##
	if [ $error_count -gt 0 ];
	then
		echo -e "\033[1;37;41m$lang_errors: $error_count\033[0m"
	else
		echo -e "\033[1;37;42m...$lang_no_errors...\033[0m"
	fi
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
		## Are we switching pools?
		## Only do this if we're not in failover, cos if we are we'll be switching anyway...
		##
		if [ $switch_pools = true ] && [ $current_pool_url = $pool_url ];
		then
			log_to_file "$lang_switching_pools..." true null true
			
			sw_pool_url=$fo_pool_url
			sw_pool_algo=$fo_pool_algo
			sw_pool_user=$fo_pool_user
			sw_pool_password=$fo_pool_password
			sw_pool_flags=$fo_pool_flags
			
			fo_pool_url=$pool_url
			fo_pool_algo=$pool_algo
			fo_pool_user=$pool_user
			fo_pool_password=$pool_password
			fo_pool_flags=$pool_flags
			
			pool_url=$sw_pool_url
			pool_algo=$sw_pool_algo
			pool_user=$sw_pool_user
			pool_password=$sw_pool_password
			pool_flags=$sw_pool_flags
			
			poolswitch_count=$((poolswitch_count+1))
		fi
		##
		## Restart ccMiner
		##
		##ENDSTAMP=$(reset_session)
		failover=false
		start_ccminer
		##session_start=$(date "+%s")
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
