.text                           # IMPORTANT: subsequent stuff is executable
.global scale_from_ports
        
## ENTRY POINT FOR REQUIRED FUNCTION
scale_from_ports:

        movl $0, %ecx                            #make sure register cx is 0 (clear it )
        movl SCALE_SENSOR_PORT(%rip), %ecx       #move scale_sensor_port to reg cx
        movl %ecx, %eax                         #move ecx to eax (return reg) so it returns scale_sensor_port

        movl $0,%edx                             #make sure dx is 0(clear dx)
        movl SCALE_TARE_PORT(%rip), %edx         #move scale_tare_port to reg dx

        cmpw $999,%cx                           # compare SCALE_SENSOR_PORT to 999)
        jg .mode_error                          # if SCALE_SENSOR_PORT >999, mode_error
        
        cmpw $0, %cx                            # compare SCALE_SENSOR_PORTto 0 
        jl .mode_error                          # if SCALE_SENSOR_PORT<0, mode_error

        cmpw $999,%dx                          #compareSCALE_TARE_PORTto 999)
        jg .mode_error                          #if SCALE_TARE_PORT >999, mode_error

        cmpw $0, %dx                           #compare SCALE_TARE_PORT to 0 
        jl .mode_error                         #if SCALE_TARE_PORT <0, mode_error

        movb $0, %sil                           #make sue reg sil is zero
        movb SCALE_STATUS_PORT(%rip),%sil      #move SCALE_STATUS_PORT to it 

        testb $32, %sil                        #see if the fifth bet in SCALE_STATUS_PORT is set (32-> 00100000)
        jnz .tare_on                            #0-> not set, not zero -> set, so not zero -> make set 

        #Here tare is not on, and mode is not error, so mode_show is on, normal procedure
        movb $1,  2(%rdi)  #moves a 1 to scale->mode -> 1 = mode_show

        
        subl %edx, %eax  #SCALE_SENSOR_PORT -= SCALE_TARE_PORT;
        movw %ax, (%rdi) #setting weight to (SCALE_SENSOR_PORT -= SCALE_TARE_PORT)

        testb $4, %sil #check if 2nd bit is set
        jnz .set_to_pounds

        #scale->indicators = 1 << 0;  #Set ounce indicator
        movb $1, 3(%rdi)

        cmpw $0, %dx  #SCALE_TARE_PORT > 0 
        jg .tare_on_special_case #scale->indicators = scale->indicators | (1 << 2);

        movl $0, %eax
        ret 

.mode_error:
        movw $0 , 0(%rdi)        # weight = 0
        movb $4,  2(%rdi)        # moves a 4 to scale->mode -> 4 = mode_error
        movb $0,  3(%rdi)       # indicators = 0 
        movl $1, %eax           #return 1 per function in c request 
        ret

.tare_on:
        movw $0 , 0(%rdi)        # weight = 0
        movb $2,  2(%rdi)        # moves a 2 to scale->mode -> 2 = mode_tare
        movb $0,  3(%rdi)        # indicators = 0 
        movl $2, %eax            #return 2 per function in c request 
        ret

// Check if the unit is set to pounds
.set_to_pounds:
        movb $2, 3(%rdi)         #this does this  ->  scale->indicators = 1 << 1; // Set pound indicator
        addl $8, %eax            #adds 8 to weight holding register also return (eax)
        movw %ax,(%rdi)          #this moves the new +8 weight to scale->weight
        sarw $4, %ax             #shift right by 4 to weight, convert from ounces to pounds
        movw %ax,(%rdi)          #this moves the new converted lbs to scale->weight

        movl $0, %eax
        ret 

.tare_on_special_case:
        movb $5, 3(%rdi) # returns an indicator of 101
        movl $0, %eax 
        ret 


.text
.global  scale_display_special

## ENTRY POINT FOR REQUIRED FUNCTION
scale_display_special:
        ## assembly instructions here

        sarl $16, %edi          #shift right by 16 to access the scale.mode
        cmpb $4, %dil           #if mode = mode_error
        je .display_err         

        cmpb $2, %dil           #if mode = mode_tare
        je .display_stor

        movl $1, %eax           #return 1 if not mode_error or mode_tare
        ret

.display_err:
        movl $0b0110111101111110111110000000, (%rsi) #this is the binary value for ERR.0
        movl $0, %eax           #return 1 if not mode_error or mode_tare
        ret

.display_stor:
        movl $0b1100111100100111110111011111, (%rsi)   #this is the binary value for STO.R
        movl $0, %eax           #return 1 if not mode_error or mode_tare
        ret


.data
patterns_array:
        .int 0b1111011  # 0
        .int 0b1001000  # 1
        .int 0b0111101  # 2
        .int 0b1101101  # 3
        .int 0b1001110  # 4
        .int 0b1100111  # 5
        .int 0b1110111  # 6
        .int 0b1001001  # 7
        .int 0b1111111  # 8
        .int 0b1101111  # 9
        .int 0b0000100  # negative sign

.text
.global scale_display_weight

scale_display_weight:
        pushq %rbx  # Will store negative flag
        pushq %r12  # Will store hundreds digit
        pushq %r13  # Will store tens digit
        pushq %r14  # Will store indicators
        
        # Check mode
        movl %edi, %eax
        shrl $16, %eax
        andl $0xFF, %eax
        cmpl $1, %eax
        jne .error
        
        # Initialize
        movzwl %di, %ecx        # weight
        movl $0, %r10d          # temp_display
        xorl %ebx, %ebx         # negative flag
        
        # Extract indicators (bits 24-27 of edi)
        movl %edi, %r14d
        shrl $24, %r14d         # r14d now contains indicators in bits 0-3
        andl $0x0F, %r14d       # Isolate 4 bits
        
        # Handle negative
        testw %cx, %cx
        jns .not_negative
        negw %cx
        movl $1, %ebx
        
.not_negative:
        # Load patterns array
        leaq patterns_array(%rip), %r11
        
        # Process digits (123 becomes 1, 2, 3)
        movl %ecx, %eax
        movl $10, %edi
        xorl %edx, %edx
        divl %edi               # eax = weight/10, edx = weight%10 (units digit)
        movl %edx, %r8d         # Save units digit
        xorl %edx, %edx
        divl %edi               # eax = weight/100, edx = weight%10 (tens digit)
        movl %edx, %r9d         # Save tens digit
        movl %eax, %r12d        # Save hundreds digit
        
        # Build display pattern
        # Check if we have hundreds digit
        cmpl $0, %r12d
        je .two_digits
        
        # Case 1: 3-digit number (1XX) - show all three digits
        movl (%r11, %r12, 4), %eax  # Hundreds digit
        shll $14, %eax
        orl %eax, %r10d
        
        # Place negative sign in bits 21-27 if needed
        testl %ebx, %ebx
        jz .three_digit_tens
        movl 40(%r11), %eax      # Negative sign
        shll $21, %eax
        orl %eax, %r10d
        
.three_digit_tens:
        # Tens digit (7-13)
        movl (%r11, %r9, 4), %eax
        shll $7, %eax
        orl %eax, %r10d
        jmp .units_digit
        
.two_digits:
        # Case 2: 2-digit number (or 1-digit padded to 2 digits)
        # Always show at least 2 digits (with leading zero if needed)
        
        # Place negative sign in bits 14-20 if needed
        testl %ebx, %ebx
        jz .two_digit_tens
        movl 40(%r11), %eax      # Negative sign
        shll $14, %eax
        orl %eax, %r10d
        jmp .two_digit_tens
        
.two_digit_tens:
        # Tens digit (7-13) - show even if zero
        movl (%r11, %r9, 4), %eax
        shll $7, %eax
        orl %eax, %r10d
        
.units_digit:
        # Units digit (0-6)
        movl (%r11, %r8, 4), %eax
        orl %eax, %r10d
        
.add_indicators:
        # Clear upper indicator bits (28-31)
        andl $0x0FFFFFFF, %r10d
        
        # Set indicators (bits 28-31) from extracted indicators
        movl %r14d, %eax
        shll $28, %eax         # Shift to bits 28-31
        orl %eax, %r10d
        
        movl %r10d, (%rsi)     # Store final display
        xorl %eax, %eax        # Return 0 (success)
        jmp .done
        
.error:
        movl $1, %eax          # Return 1 (error)
        
.done:
        popq %r14
        popq %r13
        popq %r12
        popq %rbx
        ret

.global scale_update
## ENTRY POINT FOR REQUIRED FUNCTION
scale_update:
    ## assembly instructions here
    ## DON'T FORGET TO RETURN FROM FUNCTIONS
    # Prologue: Allocate 24 bytes on stack
    subq    $24, %rsp

    # Call scale_from_ports(&temp)
    leaq    12(%rsp), %rdi      # Arg1 = address of temp.weight
    call    scale_from_ports

    # Check if mode == 2 (tare mode)
    cmpb    $2, 14(%rsp)        # temp.mode at offset 14
    je      handle_tare_mode

normal_flow:
    # Try to display special message first
    leaq    8(%rsp), %rsi       # Arg2 = &display_buffer
    movl    12(%rsp), %edi      # Arg1 = temp.weight
    call    scale_display_special
    testl   %eax, %eax          ## Check if return value == 0
    jnz     display_weight      # If special display failed, show weight

update_display:
    # Output to hardware
    movl    8(%rsp), %eax       # Load display pattern
    movl    %eax, SCALE_DISPLAY_PORT(%rip)
    xorl    %eax, %eax          # Return 0 (success)
    jmp     cleanup

handle_tare_mode:
    # Store current sensor reading as tare weight
    movzwl  SCALE_SENSOR_PORT(%rip), %eax
    movw    %ax, SCALE_TARE_PORT(%rip)
    jmp     normal_flow

display_weight:
    # Display weight if special display wasn't shown
    leaq    8(%rsp), %rsi       # Arg2 = &display_buffer
    movl    12(%rsp), %edi      # Arg1 = temp.weight
    call    scale_display_weight
    testl   %eax, %eax           ## Check if return value == 0
    je      update_display      # If success, update display
    
    # Error case
    movl    $1, %eax           # Return 1 (error)

cleanup:
    # Epilogue: Restore stack
    addq    $24, %rsp
    ret