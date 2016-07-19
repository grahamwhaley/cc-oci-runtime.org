.. contents::
.. sectnum::

``Installing Clear Containers 2.0 into Clear Linux``
====================================================

Introduction
------------
.. attention::
   This document is a work in progress - at various points you should find **Attention!** boxes, and we should clear all of these down **before** we release

`Clear Containers`_ 2.0 provides an Open Containers Initiative (OCI_) compatible 'runtime' and is installable into Docker_ 1.12-rc4 and later, where OCI_ runtime support is available.

This document details how to install Docker-1.12-rc4 and the necessary parts of `Clear Containers`_  into a `Clear Linux`_ distribution.

You will need to have a `Clear Linux`_ installation before commencing this procedure, although `Clear Containers`_ do not depend on `Clear Linux`_ as a host and can be run on top of other distributions.


Overview
--------
The following steps install and configure `Clear Containers`_ and Docker_ into an existing `Clear Linux`_ distribution. You will require `Clear Linux`_ version 8620 or above.
Again, please note that `Clear Containers`_ can run on top of other distributions. Here `Clear Linux`_ is used as one example of a distribution `Clear Containers`_ can run on.

.. attention::
   Need to check CL version - do we need the version that gets Docker 1.14-rc4 and CC2.0 integrated into its bundles for instance - which will be >9250 at least.

.. attention::
   Do we support this under a VM, and shall we doucment that here? - for instance CL under KVM under another distro? That may be a common use case to test out CL and CC.

After this installation you will be able to launch Docker_ container payloads using either the default Docker_ (``runc``) Linux Container runtime or the `Clear Containers`_ QEMU/KVM hypervisor based runtime - ``clr-oci-runtime``.

Installation Steps
------------------

Enable sudo
~~~~~~~~~~~

You will need root privileges in order to run a number of the following commands. It is recommended you run these commands from a user account with ``sudo`` rights. 

If your user does not already have ``sudo`` rights, you should add your user to wheel group whilst logged in as ``root``:

  ::

    usermod -G wheel -a <USERNAME>

And you will also need to add your user or group to the ``/etc/sudoers`` file, for example:

  ::

    visudo
    #and add the line:
      %wheel  ALL=(ALL)    ALL

You can now log out of ``root``, log in as your <USERNAME> and continue by using ``sudo``.

Check Clear Container compatability
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Before you try to install and run `Clear Containers`_ it is prudent to check that your machine (hardware) is compatible. The easiest way to do this is to download and run the Clear Containers check config script:

  ::

    curl -O https://download.clearlinux.org/current/clear-linux-check-config.sh
    chmod +x clear-linux-check-config.sh
    ./clear-linux-check-config.sh container

This command will print a list of test results. All items should return a 'SUCCESS' status - but you can ignore the 'Nested KVM support' item if it fails - this just means you cannot run `Clear Containers`_ under another hypervisor such as KVM, but can still run `Clear Containers`_ directly on top of native `Clear Linux`_ or any other distribution.

Update your Clear Linux
~~~~~~~~~~~~~~~~~~~~~~~

.. attention::
   We need to check if this is really version 8620, or a newer one where the named bundles are available.

The more recent your version of `Clear Linux`_ the better `Clear Containers`_ will perform, and the general recommendation is that you ensure that you are on the latest version of Clear Linux, or at least version 8620.

To update your `Clear Linux`_ installation to the latest execute:

  ::

    sudo swupd update

Uninstall container-basic
~~~~~~~~~~~~~~~~~~~~~~~~~

.. attention::
   Update this to reflect the state of affairs for the release - at that point the container-basic may have been updated to the most recent Docker??

If you are on an older version of `Clear Linux`_ you may have an old version of the ``container-basic`` bundle installed, that containers an older version of Docker_. If so, remove this bundle:

  ::

    sudo swupd bundle-remove containers-basic

Install Clear Containers bundles
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. attention::
   I believe this whole section can go once we have the bundles updated in Clear Linux

   We do need to list the bundles that are required for installation though - we can either do piecemeal - just CC (cor and qemu) here, and Docker later, or we can do them all at once.

Install the following bundles and RPMs to enable our work in progress linux-container-testing packages.

  ::

    sudo swupd bundle-add os-clr-on-clr
    sudo swupd bundle-add os-core-dev
    sudo swupd bundle-add os-dev-extras
    sudo rpm -ivh --nodeps --force https://download.clearlinux.org/current/x86_64/os/Packages/qemu-lite-bin-2.6.0-17.x86_64.rpm
    sudo rpm -ivh --nodeps --force https://download.clearlinux.org/current/x86_64/os/Packages/qemu-lite-data-2.6.0-17.x86_64.rpm
    sudo rpm -ivh --nodeps --force https://download.clearlinux.org/current/x86_64/os/Packages/json-glib-dev-1.2.0-8.x86_64.rpm
    sudo rpm -ivh --nodeps --force https://download.clearlinux.org/current/x86_64/os/Packages/json-glib-lib-1.2.0-8.x86_64.rpm
    sudo rpm -ivh --nodeps --force https://download.clearlinux.org/current/x86_64/os/Packages/linux-container-testing-4.5-9.x86_64.rpm
    sudo rpm -ivh --nodeps --force https://download.clearlinux.org/current/x86_64/os/Packages/linux-container-testing-extra-4.5-9.x86_64.rpm
    #Note: Ignore the errorldconfig:*
    #/usr/lib64/libguile-2.0.so.22.7.2-gdb.scm is not an ELF file - it has the wrong magic bytes at the start.*
    #Note (if you want the bleeding edge even for the first 3): Use swupd bundle-add <XXX> -u http://clearlinux-sandbox.jf.intel.com/update -F staging*

Check QEMU-lite
~~~~~~~~~~~~~~~

`Clear Containers`_ uses an optimised version of `QEMU`_ called `QEMU-lite`_
You can now check that the `QEMU-lite`_ package is installed and functioning:

  ::

    # qemu-lite-system-x86_64 --version
    QEMU emulator version 2.6.0, Copyright (c) 2003-2008 Fabrice Bellard

    # qemu-lite-system-x86_64 --machine help | grep pc-lite
    pc-lite Light weight PC (alias of pc-lite-2.6)

    pc-lite-2.6Light weight PC

.. attention::
   Should we do a run check on 'cor' here as well??


Install the Docker bundle
~~~~~~~~~~~~~~~~~~~~~~~~~

We can now install the `Clear Linux`_ bundle that containers Docker_:

.. attention::
   Here we should list the bundles required to get Docker installed

  ::

    sudo swupd bundle-add opencontainers-dev -u http://clearlinux-sandbox.jf.intel.com/update -F staging

Add your user to the KVM and Docker groups
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To enable your user to access both Docker and KVM you will need to add them to the relevant groups on the machine:
 
  ::

    sudo usermod -G kvm,docker -a <USERNAME>

Download the CC 2.0 code
~~~~~~~~~~~~~~~~~~~~~~~~

.. attention::
   This should have been done by a bundle already - as a binary install
   I think we can probably **delete** this section?

  ::

    cor_source=${HOME}/clr-oci-runtime
    git clone from_somewhere
    cd $cor_source
    autoreconf -fvi
    bash autogen.sh --disable-cppcheck --disable-valgrind
    make
    sudo make install

Setup docker proxy
~~~~~~~~~~~~~~~~~~

.. attention::
   This is Intel internal specific I believe? - if so, confirm and **delete** this section if there needs to be no equivalent for an external party.

  ::

    sudo mkdir -p /lib/systemd/system/docker-upstream.service.d/
    cat << EOF | sudo tee -a /lib/systemd/system/docker-upstream.service.d/proxy.conf
    [Service]
    Environment=add_your_proxy_server_here
    EOF

Restart Docker
~~~~~~~~~~~~~~

In order to ensure you are running the latest installed Docker_ you should restart the Docker_ daemon:

.. attention::
   Would this not be handled by the bundle post-installer in our release instance? If so, let's delete this step.

  ::

    sudo systemctl daemon-reload
    sudo systemctl restart docker-upstream

Final Docker sanity check
~~~~~~~~~~~~~~~~~~~~~~~~~

Before we dive into using `Clear Containers`_ it is prudent to do a final sanity check to ensure that relevant Docker_ parts have installed and are executing correctly:

.. attention::
   We need to put the example output text in this section.

  ::

    sudo systemctl status docker-upstream
    docker-upstream ps
    docker-upstream network ls
    docker-upstream pull debian
    docker-upstream run -it debian

If these tests pass then you have a working Docker_, and thus a good baseline to evaluate `Clear Containers`_ under.

Enable Clear Containers runtime
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Now we have `Clear Containers`_ and Docker_ installed we need to tie them together by enabling the `Clear Containers`_ runtime within the Docker_ system:

Locate where your OCI runtime got installed

    ::

      which clr-oci-runtime
      #typically /usr/bin/clr-oci-runtime

Then edit the Docker_ systemd unit file ExecStart to make `Clear Containers`_ the default runtime.

.. attention::
   Does it matter where in the file we add this - is that file empty by default?

  ::

    Edit: /usr/lib/systemd/system/docker-upstream.service
    ExecStart=/usr/bin/dockerd-upstream --add-runtime cor=/usr/bin/clr-oci-runtime--default-runtime=cor -H fd://


Install the Clear Container container kernel image
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. attention::
   fix the FIXME in the below paragraph - which bundle installed this?

`Clear Containers`_ utilise a root filesystem and Linux kernel image to run the Docker_ container payloads. The root filesystem was installed by the ``FIXME`` bundle. The kernel image can be obtained from the `Clear Linux`_ download site:

.. attention::
   This needs modifying to cover the bundle and also the correct download URL

  ::

    sudo mkdir -p /var/lib/clr-oci-runtime/data/{image,kernel}
    cd /var/lib/clr-oci-runtime/data/image/
    sudo curl -O some_magic_place
    sudo unxz clear-8900-containers.img.xz
    sudo mv clear-8900-containers.img clear-containers.img
    sudo cp /usr/lib/kernel/vmlinux-4.5-9.container.testing /var/lib/clr-oci-runtime/data/kernel/vmlinux.container
    sudo cp $cor_source/data/hypervisor.args /usr/share/defaults/clr-oci-runtime/

Restart Docker Again
~~~~~~~~~~~~~~~~~~~~

In order for the changes to take effect (and verify that the new parameters are in effect) we need to restart the Docker_ daemon again:

.. attention::
   Will this be called docker-upstream still when we have the official bundles?

  ::

    sudo systemctl daemon-reload
    sudosystemctl restart docker-upstream
    sudosystemctl status docker-upstream

Verify the runtime
~~~~~~~~~~~~~~~~~~

You can now verify that you can launch Docker_ containers with the `Clear Containers`_ runtime:
 
  ::

    sudo docker-upstream run -it debian

.. attention::
   We need to show the expected result here. Maybe we can instead start a 'uname -a' or similar as that is simpler and will visibly show we are running a CC kernel

Conclusion
----------

You now have Docker_ installed with `Clear Containers`_ enabled as the default OCI_ runtime. You can now try out `Clear Containers`_.

.. _Clear Containers: https://clearlinux.org/features/clear-containers

.. _Clear Linux: www.clearlinux.org

.. _Docker: https://www.docker.com/

.. _OCI: https://www.opencontainers.org/

.. _QEMU: http://wiki.qemu.org/Main_Page

.. _QEMU-lite: http://github.com/01org/qemu-lite

