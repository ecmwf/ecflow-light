!
! (C) Copyright 2023- ECMWF.
!
! This software is licensed under the terms of the Apache Licence version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
! In applying this licence, ECMWF does not waive the privileges and immunities
! granted to it by virtue of its status as an intergovernmental organisation
! nor does it submit to any jurisdiction.
!

program ecflow_light_client

    use ecflow_light

    implicit none

    character(256) :: option
    character(256) :: name
    character(256) :: value_str
    integer :: value_int
    integer :: error

    call GET_COMMAND_ARGUMENT(1, option)
    call GET_COMMAND_ARGUMENT(2, name)
    call GET_COMMAND_ARGUMENT(3, value_str)

    if (option == '--meter') then
        read (value_str, *) value_int
        error = ecflow_light_update_meter(name, value_int)
    else if (option == '--label') then
        error = ecflow_light_update_label(name, value_str)
    else if (option == '--event') then
        if (value_str == 'set') then
            value_int = 1
        else if (value_str == 'clear') then
            value_int = 0
        else
            print *, 'ERROR: Invalid value for event.'
            call exit(1)
        end if
        error = ecflow_light_update_event(name, value_int)
    else
        print *, 'ERROR: Unknown option detected'
        call exit(1)
    end if

    call exit(0)

end program ecflow_light_client
