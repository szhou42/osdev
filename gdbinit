Here is some useful gdb shortcut when debugging os, for example, when a triple fault happens, you want to set breakpoint on every exception handler and catch the first exception before a triple
fault can happen. Also you can dumo memory like xxd in gdb using commands "xxd var size"(for example xxd str 10)
define hook-quit
    set confirm off
end

define astep
    stepi
    disas
end

define astep2
    stepi
    x/10i $eip
end

define b_exception
    b exception0
    b exception1
    b exception2
    b exception3
    b exception4
    b exception5
    b exception6
    b exception7
    b exception8
    b exception9
    b exception10
    b exception11
    b exception12
    b exception13
    b exception14
    b exception15
    b exception16
    b exception17
    b exception18
    b exception19
    b exception20
    b exception21
    b exception22
    b exception23
    b exception24
    b exception25
    b exception26
    b exception27
    b exception28
    b exception29
    b exception30
    b exception31
    b exception128
end

define xxd
    dump binary memory dump.bin $arg0 $arg0+$arg1
    shell xxd dump.bin
end
