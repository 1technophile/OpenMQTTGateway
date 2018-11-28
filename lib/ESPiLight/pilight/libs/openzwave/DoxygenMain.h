
/**
 * \mainpage OpenZWave
 * \section Introduction Introduction to OpenZWave
 * OpenZWave is an open-source, cross-platform library designed to enable
 * anyone to add support for Z-Wave home-automation devices to their
 * applications, without requiring any in depth knowledge of the Z-Wave
 * protocol.
 * <p>
 * Z-Wave employs a proprietary protocol which the owners, Sigma Designs,
 * have chosen not to release into the public domain.  There is also no
 * official free or low-cost SDK that can be used to develop applications
 * (The ControlThink SDK is now tied exclusively to their own Z-Wave PC
 * interface).  The only way to obtain the protocol documentation and sample
 * code is to purchase an expensive development kit, and sign a non-disclosure
 * agreement (NDA) preventing the release of that knowledge.
 * <p>
 * OpenZWave was created to fill that gap.  We do not have the official
 * documentation, have signed no NDA, and are free to develop the library as
 * we see fit.  Our knowledge comes from existing bodies of open-source code
 * (principally the Z-Wave parts of LinuxMCE), and through examining the
 * messages sent by Z-Wave devices.
 * <p>
 * The goal of the project is to make a positive contribution to the Z-Wave
 * community by creating a library that supports as much of the Z-Wave
 * specification as possible, and that can be used as a "black-box"
 * solution by anyone wanting to add Z-Wave to their application.  It is NOT
 * our aim to publish alternative documentation of the Z-Wave protocol, or
 * to attempt to "punish" Sigma Designs for their decision to keep the
 * protocol closed.

 * \section ZWave Z-Wave Concepts
 * Z-Wave is a proprietary wireless communications protocol employing mesh
 * networking technology.  A mesh network allows low power devices to
 * communicate over long ranges, and around radio blackspots by passing
 * messages from one node to another.  It is important to note that not all
 * Z-Wave devices are active all the time, especially those that are battery
 * powered.  These nodes cannot take part in the forwarding of messages
 * across the mesh.
 * <p>
 * Each Z-Wave device is known as "Node" in the network.  A Z-Wave network
 * can contain up to 232 nodes.  If more devices are required, then
 * multiple networks need to be set up using separate Z-Wave controller.
 * OpenZWave supports multiple controllers, but on its own does not bridge
 * the networks allowing a device on one to directly control a device on
 * another.  This functionality would have to be supplied by the application.
 * <p>
 * Z-Wave nodes can be divided into two types: Controllers and Slaves.  The
 * controllers are usually in the form of hand-held remote controls, or PC
 * interfaces.  The switches, dimmers, movement sensors etc are all slaves.
 * Controllers and Devices
 * Replication
 * Command Classes
 * Values

 * <hr>
 * \section Library The OpenZWave Library
 * \subsection Overview Overview

 * \subsection Manager The Manager
 * All Z-Wave functionality is accessed via the Manager class.  While this
 * does not make for the most efficient code structure, it does enable
 * the library to handle potentially complex and hard-to-debug issues
 * such as multi-threading and object lifespans behind the scenes.
 * Application development is therefore simplified and less prone to bugs.
 * <p>

 * \subsection Notifications Notifications
 * Communication between the PC and devices on the the Z-Wave network occurs
 * asynchronously.  Some devices, notably movement sensors, sleep most of the
 * time to save battery power, and can only receive commands when awake.
 * Therefore a command to change a value in a device may not occur immediately.
 * It may take several seconds or minutes, or even never arrive at all.
 * For this reason, many OpenZWave methods, even those that request
 * information, do not return that information directly.  A request will be
 * sent to the network, and the response sent to the application some time
 * later via a system of notification callbacks.  The notification handler
 * will be at the core of any application using OpenZWave.  It is here that
 * all information regarding device configurations and states will be reported.
 * <p>
 * A Z-Wave network is a dynamic entity.  Controllers and devices can be
 * added or removed at any time.  Once a network has been set up, this
 * probably won't happen very often, but OpenZWave, and any application
 * built upon it, must cope all the same.  The notification callback system
 * is used to inform the application of any changes to the structure of the
 * network.

 * \subsection Values Working with Values
 *

 * \subsection Guidelines Application Development Guidelines
 * The main considerations when designing an application based upon
 * OpenZWave are:
 *
 * - Communication with Z-Wave devices is asynchronous and not guaranteed
 * to be reliable.  Do not rely on receiving a response to any request.
 *
 * - A Z-Wave network may change at any time.  The application's notification
 * callback handler must deal woth all notifications, and any representation
 * of the state of the Z-Wave network held by the application must be
 * modified to match.  User interfaces should be built dynamically from the
 * information reported in the notification callbacks, and must be able to cope
 * with the addition and removal of devices.
 *
 * Some users will have more than one Z-Wave controller, to allow for remote
 * locations or to work around the 232 device limit.  OpenZWave is designed
 * for use with multiple controllers, and all applications should be written
 * to support this.
 *
 * - There is no need to save the state of the network and restore it on
 * the next run. OpenZWave does this automatically, and is designed to cope
 * with any changes that occur to the network while the application is not
 * running.
 *

 * <hr>
 * \section Structure Source Code Structure
 *

 * <hr>
 * \section Samples Samples
 * The SDK package includes skeleton code for creating xPL-enabled
 * applications.  Code is provided for three types of application - a Windows
 * service, a Windowed application, and a console application.  The Windows
 * service is quite flexible - it will run as a console application if no
 * command line parameters are passed to it.  This makes it possible to
 * ship one application that will run on all versions of Windows, whether or
 * not there is support for services.  It is also useful for debugging.
 * <p>
 * The full source code for the W800RF32 service is also provided as a real
 * life example of using the SDK to build a complete xPL application.

 * <hr>
 * \section Licensing Licensing
 * OpenZWave is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 * <p>
 * OpenZWave is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * <p>
 * You should have received a copy of the GNU Lesser General Public License
 * along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.

 * <hr>
 * \section Support Support
 * Assistance with all OpenZWave issues can be obtained by posting a message
 * to the OpenZWave Google Group (http://groups.google.com/group/openzwave)
 * <p>
 */

