!
! (C) Copyright 2023- ECMWF.
!
! This software is licensed under the terms of the Apache Licence version 2.0
! which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
! In applying this licence, ECMWF does not waive the privileges and immunities
! granted to it by virtue of its status as an intergovernmental organisation
! nor does it submit to any jurisdiction.
!

module ecflow_light

interface

    function ecflow_light_update_meter_f_api(name, value) result(error) &
            bind(C, name = 'ecflow_light_update_meter')

        use iso_c_binding, only : c_char, c_int
        implicit none

        character(c_char), intent(in) :: name(*)
        integer(c_int), intent(in), value :: value
        integer(c_int) :: error

    end function

    function ecflow_light_update_label_f_api(name, value) result(error) &
            bind(C, name = 'ecflow_light_update_label')

        use iso_c_binding, only : c_char, c_int
        implicit none

        character(c_char), intent(in) :: name(*)
        character(c_char), intent(in) :: value(*)
        integer(c_int) :: error

    end function

    function ecflow_light_update_event_f_api(name, value) result(error) &
            bind(C, name = 'ecflow_light_update_event')

        use iso_c_binding, only : c_char, c_int
        implicit none

        character(c_char), intent(in) :: name(*)
        integer(c_int), intent(in), value :: value
        integer(c_int) :: error

    end function

end interface

contains

    function ecflow_light_update_meter(name, value) result(error)

        implicit none
        character(*), intent(in) :: name
        integer, intent(in), value :: value
        integer :: error

        error = ecflow_light_update_meter_f_api(str_fortran_to_c(name), value)

    end function

    function ecflow_light_update_label(name, value) result(error)

        implicit none
        character(*), intent(in) :: name
        character(*), intent(in) :: value
        integer :: error

        error = ecflow_light_update_label_f_api(str_fortran_to_c(name), str_fortran_to_c(value))

    end function

    function ecflow_light_update_event(name, value) result(error)

        implicit none
        character(*), intent(in) :: name
        integer, intent(in), value :: value
        integer :: error

        error = ecflow_light_update_event_f_api(str_fortran_to_c(name), value)

    end function

    function str_fortran_to_c(string_in) result(string_out)

        implicit none
        character(*), intent(in) :: string_in
        character(:), allocatable :: string_out

        string_out = trim(string_in) // char(0)

    end function str_fortran_to_c

end module ecflow_light
