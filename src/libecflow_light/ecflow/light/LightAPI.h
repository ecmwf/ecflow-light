/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_LIGHT_LIGHTAPI_H
#define ECFLOW_LIGHT_LIGHTAPI_H

#include <string>

namespace ecflow::light {

/**
 *  Initialize library based on given configuration (i.e. ECF_UDP_HOST, ECF_UDP_PORT).<br>
 *  <br>
 *  The following environment variables will be used, if available:
 *  <ul>
 *    <li><em>ECF_UDP_HOST</em>, defines the hostname of `ecflow_udp` to use</li>
 *    <li><em>ECF_UDP_PORT</em>, defines the port to use</li>
 *    <li><em>IFS_ECF_CONFIG_PATH</em>, defines the path to a YAML configuration
 *        file to be loaded (n.b. parameters in the configuration file
 *        override the environment variables, such as ECF_UDP_HOST/PORT)</li>
 *  </ul>
 */
void init();

/** Updates the named meter with the given value.<br>
 *  <br>
 *  The update of the meter is performed by sending a request (UDP or HTTP) to
 *  `ecflow_udp/http` define during the initial configuration. The request is handled
 *  by `ecflow_server` as if it originates from a child task.<br>
 *  <br>
 *  The following mandatory environment variables must be defined:
 *  <ul>
 *    <li><em>ECF_RID</em>, defines the child task remote id</li>
 *    <li><em>ECF_NAME</em>, defines the child task name, which is effectively
 *        the task's path in the suite</li>
 *    <li><em>ECF_PASS</em>, defines the child task password, determined by
 *        `ecflow_server` when submitting the task</li>
 *    <li><em>ECF_TRYNO</em>, defined a sequence execution number, determined
 *         by `ecflow_server` when submitting the task</li>
 *  </ul>
 *  IMPORTANT:
 *    In case any of these variables is missing, the update request will fail.
 * 
 *  @param name the name of the meter to update
 *  @param value the new value of the meter
 * 
 *  @return <em>EXIT_SUCCESS</em> when request what handled successfully;
 *          otherwise, <em>EXIT_FAILURE</em>.
 */
int child_update_meter(const std::string& name, int value);
int child_update_label(const std::string& name, const std::string& value);
int child_update_event(const std::string& name, bool value);

}  // namespace ecflow::light

#endif
