Reference
================================================================================

Configuration
--------------------------------------------------------------------------------

The ecFlow Light configuration is provided as a YAML configuration file and
describes the clients (and associated communication mechanisms) to be used.
Multiple clients can be configured simultaneously. The library uses the value
of the ``ICF_ECFLOW_CONFIG_PATH`` environment variable to locate the
configuration file.

ecFlow Light configuration is loaded only once, on the first call to a function
from the library API.

The following communication mechanisms are currently supported:

- send telemetry update using UDP, without forking spawning any process
- send telemetry update using TCP, by effectively calling the CLI ecflow_client
  and thus spawning a new independent process

.. code-block::
   :caption: ecFlow Light configuration example

    ---
    # Clients for ecFlow Server
    clients:
    - kind: library
      protocol: udp
      host: $ENV{ECF_HOST}
      port: 8080
      version: 1

    - kind: cli
      protocol: tcp
      version: 1

    - kind: phony
      protocol: none
      version: 1

Apart from the YAML configuration, ecFlow Light also collects information from
execution context of the task by consulting the value of the following
environment variables:

- ``NO_ECF``, when detected the configuration is ignored and a *phony* client
  is setup
- ``ECF_NAME``, the path to the task associated with the attribute (meter,
  label, event) to be updated
- ``ECF_RID``, the remote identifier (i.g. PID) of the task
- ``ECF_PASS``, the password assigned to the task
- ``ECF_TRYNO``, the *try number* assigned to the particular task execution

C API
--------------------------------------------------------------------------------

The following functions are available when using :code:`#include <ecflow/light/API.h>`.

.. doxygenfunction:: ecflow_light_update_meter
    :project: ecflowlight


.. doxygenfunction:: ecflow_light_update_label
    :project: ecflowlight


.. doxygenfunction:: ecflow_light_update_event
    :project: ecflowlight

Fortran 90 API
--------------------------------------------------------------------------------

All C API functions are available directly Fortran 90 as part of
``module ecflow_light``.
