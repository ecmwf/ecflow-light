!
! (C) Copyright 2023- ECMWF.
!
! This software is licensed under the terms of the Apache Licence version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
! In applying this licence, ECMWF does not waive the privileges and immunities
! granted to it by virtue of its status as an intergovernmental organisation
! nor does it submit to any jurisdiction.
!

program ecflow_light_client_multiple

    use ecflow_light

    implicit none

    integer :: i, error

    character(256) :: n_steps_str
    character(256) :: step_size_str
    integer :: n_steps
    integer :: step_size
    integer :: minute
    integer :: second


    integer :: task_meter_value
    character(256) :: task_label_value
    integer :: task_event_value

    call GET_COMMAND_ARGUMENT(1, n_steps_str)
    call GET_COMMAND_ARGUMENT(2, step_size_str)

    read (n_steps_str, *) n_steps
    read (step_size_str, *) step_size

    do i = 0, n_steps - 1
        minute = (i / 60)
        second = mod(i, 60)

        write (*, '(A, I0, A, I0)') '>>> minute: ', minute, ', second: ', second

        ! send meter
        task_meter_value = second
        error = ecflow_light_update_meter('task_meter', task_meter_value)

        ! send label
        write (task_label_value, '(A, I0, A, I0, A)') 'Going on <',minute,'><',second,'>'
        error = ecflow_light_update_label('task_label', task_label_value)

        ! send event
        if (mod(second, 2) == 0) then
            task_event_value = 1
        else
            task_event_value = 0
        end if
        error = ecflow_light_update_event('task_event', task_event_value)

        call sleep(step_size)
    end do

end program ecflow_light_client_multiple
