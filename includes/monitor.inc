#!/bin/bash

get_gpu_details()
{
	declare -i timenow=$(date "+%s")
	declare -i opening_period=$((session_start + warmup_period))
	declare -a low_temp_gpu
	low_temp_triggered=false
	high_temp_triggered=false 
	lost_gpu_triggered=false

	for i in "${used_gpus[@]}"
	do
		TEMP=$(nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader,nounits --id=$i)
		FAN=$(nvidia-smi --query-gpu=fan.speed --format=csv,noheader,nounits --id=$i)
		MEM_USED=$(nvidia-smi --query-gpu=memory.used --format=csv,noheader,nounits --id=$i)
		NAME=$(nvidia-smi --query-gpu=name --format=csv,noheader,nounits --id=$i)
		##
		## Output the data
		##
		log_to_file "##################" true
		log_to_file "GPU[$i], $NAME" true
		log_to_file "$lang_temp $TEMP C, $lang_fan $FAN %, $lang_mem $MEM_USED Mib" true
		##
		## Make sure our data is good, if we don't have acceptable values for everything then we have a hardware error and need to restart ASAP
		##
		
		##if [ $lost_gpu = 0 ];
		##then
		##	TEMP="[Gpu is lost]"
		##fi
		if ! [ "$TEMP" -eq "$TEMP" >& /dev/null ]; 
		then
			## We appear to have lost a GPU
			log_to_file "$lang_error_lost_gpu_warning GPU[$i] $lang_error_lost_gpu_no_longer_seen" true "error"
			## We need to stop mining until this card recovers
			stop_ccminer
			log_to_file "$lang_error_mining_paused_for $lost_gpu_wait $lang_error_due_to_lost_gpu" true "error"
			##
			## Need to send email here
			##
			process_email "$lang_error_ccminer_stopped_for $lost_gpu_wait $lang_error_due_to_lost_gpu..." "$lang_error_alert"
			
			lost_gpu=$((lost_gpu + 1))
			lost_gpu_triggered=true
			
			##log_to_file "Lost GPU count =$lost_gpu" true "error"
			sleep $lost_gpu_wait
			
			if [ $lost_gpu -lt 2 ];
			then
				##ENDSTAMP=$(reset_session)
				start_ccminer
				##session_start=$(date "+%s")
			fi
			break
			
		fi
		
		## Temps too low?
		if [ $TEMP -lt $low_temp_limit ] && [ $timenow -gt $opening_period ];
		then
			## Set low-temp error
			log_to_file "$lang_error_low_temp_warning GPU[$i] $lang_temp = $TEMP C" true "error"
			low_temp_gpu[$i]=$TEMP
			low_temp_triggered=true					
		fi
		
		## Temps too high?
		if [ $TEMP -gt $high_temp_limit ];
		then
			## Set high-temp error
			log_to_file "$lang_error_high_temp_warning GPU[$i] $lang_temp = $TEMP C" true "error"
			## We need to stop mining until this card recovers
			stop_ccminer
			log_to_file "$lang_error_mining_paused_for $high_temp_wait $lang_error_due_to_high_temp" true "error"
			##
			## Need to send email here
			##
			process_email "$lang_error_mining_paused_for $high_temp_wait $lang_error_due_to_high_temp..." "$lang_error_alert"
			sleep $high_temp_wait
			##ENDSTAMP=$(reset_session)
			start_ccminer
			##session_start=$(date "+%s")
			high_temp_triggered=true
			break				
		fi
		
	done
	##
	## Did we have low temps (not interested if there was a high temp trigger or lost GPU, because we've reset anyway!)?
	##
	if [ $low_temp_triggered = true ] && [ $high_temp_triggered = false ] && [ $lost_gpu_triggered = false ];
	then
		## Need to assess how long this has been going on...
		low_temp_action=false
		if [ $lowtemp_time = 0 ];
		then
			## First trigger, so set the time
			lowtemp_time=$timenow
			lowtemp_trigger=$(( timenow+low_temp_ignore))
		else
			## Not the first time, so how long?
			if [ $lowtemp_trigger -lt $timenow ];
			then
				##log_to_file "Low temp trigger HAS BEEN reached!!!" true	
				low_temp_action=true			
			fi
		fi
		##log_to_file "Low Temp Time = $lowtemp_time" true
		##
		## Action required???
		##
		if [ $low_temp_action = true ];
		then
			## Check if any Gpu's are sitting healthy...
			total_gpus=${#used_gpus[@]}
			error_gpus=${#low_temp_gpu[@]}
		
			if [ $error_gpus -lt $total_gpus ];
			then
				## Not all GPU's affected, most likely a card has crashed... need to do a reset.
				stop_ccminer
				log_to_file "$lang_error_mining_paused_for $crash_wait_time $lang_error_due_to_suspected_crash_gpu" true "error"
				##
				## Need to send email here
				##
				process_email "$lang_error_mining_paused_for $crash_wait_time $lang_error_due_to_suspected_crash_gpu" "$lang_error_alert"
				sleep $crash_wait_time
				##ENDSTAMP=$(reset_session)
				start_ccminer
				##session_start=$(date "+%s")
				lowtemp_time=0
				##log_to_file "Not all GPU's are affected" true
				## Need to figure out how to remember this happened, we might just want to stop mining altogether after too many resets.
				##
			else
				##
				## All GPU's affected, need to consider a failover.
				## But first, maybe ccMiner got closed by accident?
				##
				if ps aux | grep "[c]cminer" > /dev/null
				then
					##
					## ccMiner confirmed as running...
					##
					## Invoke failover.
					##stop_ccminer
					##log_to_file "$lang_error_failover_activated..." true "error"
					##
					## Need to send email here
					##
					##process_email "$lang_error_ccminer_failover_activated" "$lang_error_alert"
					##sleep 5
					invoke_failover
					
					##ENDSTAMP=$(reset_session)
					start_ccminer true
					##session_start=$(date "+%s")
					lowtemp_time=0
				else
					##
					## ccMiner not running...
					##
					log_to_file "$lang_error_unexplained_restart..." true "error"
					##ENDSTAMP=$(reset_session)
					start_ccminer
					##session_start=$(date "+%s")
					lowtemp_time=0
				fi
				
				##log_to_file "All GPU's are affected" true
			fi		
			##for i in "${!low_temp_gpu[@]}"
			##do
			
				##log_to_file "Checked for low temp on GPU $i and temperature was ${low_temp_gpu[$i]}" true
			##done
		fi
	else
		## Reset triggers
		lowtemp_time=0
		## Did we get a successful reset on lost gpu?
		if [ $lost_gpu_triggered = false ] && [ $lost_gpu -gt 0 ];
		then
			lost_gpu=0
			log_to_file "$lang_error_lost_gpu_recover" true
			##
			## Need to send email here
			##
			process_email "$lang_error_lost_gpu_recover" "$lang_error_alert"
		fi
	fi
}
