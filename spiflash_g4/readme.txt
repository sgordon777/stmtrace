timing for test:


baseline:
3.706 / 38.277	;
3.70  / 38.041	; 
		38.0412
3.70 /  23.67   ; removing wait flash_wait_for_ready from flash_write_enable  
3.70 /  16      ; avoid extra copy of page
3.75 /  15.8647 ; CIRC_UPDDATE macro
3.70 /  15.6235 ; use LL for GPIO
1.16 /  7.2941  ; O0 to O3
