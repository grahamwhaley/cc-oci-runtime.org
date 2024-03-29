.. contents::
.. sectnum::

``clr-oci-runtime``
===================

Requirements
------------

- A Qemu_ hypervisor that supports the ``pc-lite`` machine type.

Overview
--------

``clr-oci-runtime`` is an Open Containers Initiative (OCI_) "runtime"
that launches an Intel_ VT-x secured Clear Containers 2.0 hypervisor,
rather than a standard Linux container. It leverages the highly
optimised `Clear Linux`_ technology to achieve this goal.

The tool aims to be compatible with the OCI_ runtime specification
[#oci-spec]_, allowing Clear Containers to be launched transparently by
Docker_ (using containerd_) and other OCI_-conforming container managers.

Platform Support
----------------

``clr-oci-runtime`` supports running Clear Containers on Intel 64-bit (x86-64) Linux systems.

Supported Application Versions
------------------------------

``clr-oci-runtime`` has been tested with the following application
versions:

- Docker_ version 1.12-rc4.
- Containerd_ version 0.2.2.

Running under ``docker``
------------------------

Assuming a Docker_ 1.12 environment, start the Docker_ daemon specifying
the "``--add-runtime $alias=$path``" option. For example::

    $ sudo dockerd --add-runtime cor=/usr/bin/clr-oci-runtime

Then, to run a Clear Container using ``clr-oci-runtime``, specify "``--runtime cor``". For example::

    $ sudo docker-run --runtime cor -ti busybox

Running under ``containerd`` (without Docker)
---------------------------------------------

If you are running Containerd_ directly, without Docker_:

- Start the server daemon::

    $ sudo /usr/local/bin/containerd --debug --runtime $PWD/clr-oci-runtime

- Launch a hypervisor::

    $ name=foo

    # XXX: path to directory containing the following:
    #
    # config.json
    # hypervisor.args
    # rootfs/
    $ bundle_dir=...

    $ sudo /usr/local/bin/ctr --debug containers start --attach "$name" "$bundle_dir"

- Forcibly stop the hypervisor::

    $ name=foo
    $ sudo ./clr-oci-runtime stop "$name"

Running stand-alone
-------------------

``clr-oci-runtime`` can be run directly, without the need for either
``docker`` or ``containerd``::

    $ name=foo
    $ pidfile=/tmp/cor.pid
    $ logfile=/tmp/cor.log
    $ sudo ./clr-oci-runtime --debug --log /dev/stdout start --console $(tty) --pid-file "$pidfile" "$name" "$bundle_dir"

Or, to simulate how ``containerd`` calls the runtime::

    $ sudo ./clr-oci-runtime --log "$logfile" --log-format json start --bundle "$bundle_dir" --console $(tty) -d --pid-file "$pidfile" "$name"

Running as a non-privileged user
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Assuming the following provisos, ``clr-oci-runtime`` can be run as a
non-``root`` user:

- User has read+write permissions for the Clear Containers root
  filesystem image specified in the ``vm`` JSON object (see
  Configuration_).

- User has read+execute permissions for the Clear Containers kernel
  image specified in the ``vm`` JSON object (see Configuration_).

- The bundle configuration file ("``config.json``") does not specify any
  mounts that the runtime must honour.

- The runtime is invoked with the "``--root=$dir``" option where
  "``$dir``" is a pre-existing directory that the user has write
  permission to.

To run non-privileged::

    $ name=foo
    $ dir=/tmp/cor
    $ mkdir -p "$dir"
    $ ./clr-oci-runtime --root "$dir" create --console $(tty) --bundle "$oci_bundle_directory" "$name"
    $ ./clr-oci-runtime --root "$dir" start "$name"

Building
--------

Dependencies
~~~~~~~~~~~~

Ensure you have the development versions of the following packages
installed on your system:

- check
- glib
- json-glib
- uuid

Configure Stage
~~~~~~~~~~~~~~~

Quick start, just run::

  $ ./autogen.sh && make

If you have specific requirements, run::

  $ ./configure --help

.. then add the extra configure flags you want to use::

  $ ./autogen.sh --enable-foo --disable-bar && make

Tests
-----

To run the basic unit tests, run::

  $ make check

To configure the command above to also run the functional tests, see the
`functional tests README`_.

Configuration
-------------

At the time of writing, the OCI_ had not agreed on how best to handled
VM-based runtimes such as this (see [#oci-vm-config-issue]_).

Until the OCI_ specification clarifies how VM runtimes will be defined, ``clr-oci-runtime`` will search a number of different data sources for its VM configuration information:

- It consults ``config.json`` in the bundle directory for a "``vm``" object, according to the proposed OCI specification [#oci-vm-config-issue]_

  You'll need to adjust the included ``data/config.json`` for your setup.

- If no "``vm``" object is found in ``config.json``, the file ``/etc/clr-oci-runtime/vm.json`` will be also be scanned for a "``vm``" object.

  An example of this file can be found as ``data/vm.json`` after the build has completed.

- It consults ``hypervisor.args`` in the bundle directory, which specifies all the arguments to the hypervisor, one per line.

  An example of this file can be found as ``data/hypervisor.args`` after the build has completed.

- If ``hypervisor.args`` is not found in the bundle directory, the file ``/etc/clr-oci-runtime/hypervisor.args`` will be used.

Currently, the tool will expand the following ``special tags`` found in ``hypervisor.args`` appropriately:

- ``@COMMS_SOCKET@`` - path to the hypervisor control socket (QMP socket for qemu).
- ``@CONSOLE_DEVICE@`` - hypervisor arguments used to control where console I/O is sent to.
- ``@IMAGE@`` - clr rootfs image path (read from ``config.json``).
- ``@KERNEL_PARAMS@`` - kernel parameters (from ``config.json``).
- ``@KERNEL@`` - path to kernel (from ``config.json``).
- ``@NAME@`` - VM name.
- ``@PROCESS_SOCKET@`` - required to detect efficiently when hypervisor is shut down.
- ``@SIZE@`` - size of @IMAGE@ which is auto-calculated.
- ``@UUID@`` - VM uuid.
- ``@WORKLOAD_DIR@`` - path to workload chroot directory that will be mounted (via 9p) inside the VM.

Logging
-------

The runtime logs to the file specified by the global ``--log`` option.
However, it can also write to a global log file if the
``--global-log`` option is specified. Note that if both log options are
specified, both log files will be appended to.

The global log potentially provides more detail than the standard log
since it is always written to in ASCII format and includes Process ID
details. Also note that all instances of ``clr-oci-runtime`` will append to
the global log.

The global log file is named ``clr-oci-runtime.log``, and will be written into the directory specified by "``--root``".
The default runtime state directory is ``/run/opencontainer/containers/`` if no "``--root``" argument is supplied.

Note: Global logging is presently always enabled in ``clr-oci-runtime``,
as ``containerd`` does not always invoke the runtime with the ``--log`` argument, and enabling the global log in this case helps with debugging.

Command-line Interface
----------------------

At the time of writing, the OCI_ has provided recommendations for the
runtime command line interface (CLI) (see [#oci-runtime-cli]_).

However, the OCI_ runtime reference implementation, runc_, has a CLI
which deviates from the recommendations.

This issue has been raised with OCI_ (see [#oci-runtime-cli-clarification]_), but
until the situation is clarified, ``clr-oci-runtime`` strives to
support both the OCI_ CLI and the runc_ CLI interfaces.

Details of the runc_ command line options can be found in the `runc manpage`_.

Note: The ``--global-log`` argument is unique to ``clr-oci-runtime`` at present.

Extensions
~~~~~~~~~~

list
....

The ``list`` command supports a "``--all``" option that provides
additional information including details of the resources used by the
virtual machine.

Development
-----------

Follow the instructions in `Building`_, but you will also want to install:

- doxygen
- lcov
- valgrind

To build the API documentation::

  $ doxygen Doxyfile

Then, point your browser at ``/tmp/doxygen-clr-oci-runtime``. If you
don't like that location, change the value of ``OUTPUT_DIRECTORY`` in
the file ``Doxyfile``.

Debugging
---------

- Specify the ``--enable-debug`` configure option to the ``autogen.sh``
  script which enable debug output, but also disable all compiler and
  linker optimisations.

- If you want to see the hypervisor boot messages, remove "`quiet`" from
  the hypervisor command-line in "``hypervisor.args``".

- Run with the "``--debug``" global option.

- If you want to debug as a non-root user, specify the "``--root``"
  global option. For example::

    $ gdb --args ./clr-oci-runtime \
        --debug \
        --root /tmp/cor/ \
        --global-log /tmp/global.log \
        start --console $(tty) $container $bundle_path

- Consult the global Log (see Logging_).

Links
-----

.. _Intel: https://www.intel.com

.. _`Clear Linux`: https://clearlinux.org/

.. _`Qemu`: http://qemu.org

.. _OCI: https://www.opencontainers.org/

.. _runc: https://github.com/opencontainers/runc

.. _`runc manpage`: https://github.com/opencontainers/runc/blob/master/man/runc.8.md`

.. _Docker: https://github.com/docker/docker

.. _containerd: https://github.com/docker/containerd

.. [#oci-spec]
   https://github.com/opencontainers/runtime-spec

.. [#oci-runtime-cli]
   https://github.com/opencontainers/runtime-spec/blob/master/runtime.md

.. [#oci-vm-config-issue]
   https://github.com/opencontainers/runtime-spec/pull/405

.. [#oci-runtime-cli-clarification]
   https://github.com/opencontainers/runtime-spec/issues/434

.. _`functional tests README`: https://github.com/01org/clr-oci-runtime/tree/master/tests/functional/README.rst
