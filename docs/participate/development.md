# Development contributions

We like pull requests from everyone and ask you to follow the code of conduct below.

* [Code of conduct](https://github.com/1technophile/OpenMQTTGateway/blob/master/CODE_OF_CONDUCT.md)

* [Code style guide](https://google.github.io/styleguide/cppguide.html#Formatting)

If you need a step-by-step install and build guide, read the [Quick Start to Develop OpenMQTTGateway](./quick_start.md). It explains the tools, PlatformIO, and docs workflow. Use this page as a fast checklist once you know the flow.

**Quick checklist**
1. Fork the [development branch](https://github.com/1technophile/OpenMQTTGateway/tree/development) and clone the repo.
2. Make your changes and follow the naming rules:
    * New gateway: `ZgatewayXXX` where `XXX` is the protocol name.
    * New sensor: `ZsensorYYY` where `YYY` is the sensor type.
    * New actuator: `ZactuatorZZZ` where `ZZZ` is the actuator type.
3. Review your code and compile for ESP32 and ESP8266.
4. Test on your hardware.
5. Open a pull request, verify the GitHub Actions CI build, and request a review.

## Automated Testing and CI

Your pull request will be automatically tested by GitHub Actions. If you want to run the same checks locally before pushing, you can use the `ci.sh` script in the scripts folder.

**Need help with the ci.sh commands?** See [section 5 in Quick Start](./quick_start.md#5-firmware-development-from-the-terminal-with-cish) for detailed examples of running QA checks and builds from the terminal.

For more details on how CI works and available scripts, see the [CI documentation](https://github.com/1technophile/OpenMQTTGateway/blob/development/scripts) in the scripts folder.

For a comprehensive overview of all GitHub Actions workflows used in this project, check the [Workflows README](https://github.com/1technophile/OpenMQTTGateway/tree/development/.github/workflows).

## Code Quality

To format your code automatically, add the "clang-Format" extension to VSCode, then right click in the file and choose "Format document".


We may suggest some changes, improvements or alternatives.

Some things that will increase the chance that your pull request is accepted:
* Comment your code,
* Ask eventually for design guidelines,
* Write a [good commit message][commit].

[commit]: http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html

## Nightly builds

Nightly builds can be found [here](https://docs.openmqttgateway.com/dev/upload/web-install.html) for testing purposes only.
They are generated from the development branch nightly or from a particular pull request upon request.

## Developer Certificate Of Origin

```
    By making a contribution to this project, I certify that:

    (a) The contribution was created in whole or in part by me and I
        have the right to submit it under the open source license
        indicated in the file; or

    (b) The contribution is based upon previous work that, to the best
        of my knowledge, is covered under an appropriate open source
        license and I have the right under that license to submit that
        work with modifications, whether created in whole or in part
        by me, under the same open source license (unless I am
        permitted to submit under a different license), as indicated
        in the file; or

    (c) The contribution was provided directly to me by some other
        person who certified (a), (b) or (c) and I have not modified
        it.

    (d) I understand and agree that this project and the contribution
        are public and that a record of the contribution (including all
        personal information I submit with it, including my sign-off) is
        maintained indefinitely and may be redistributed consistent with
        this project or the open source license(s) involved.
```

This Developer Certificate Of Origin (DCO) was adopted on June 7, 2021.

The text of this license is available under the [Creative Commons Attribution-ShareAlike 3.0 Unported License](http://creativecommons.org/licenses/by-sa/3.0/).  It is based on the Linux [Developer Certificate Of Origin](http://elinux.org/Developer_Certificate_Of_Origin).

To accept the DCO it is required to put a x between [ ] on `[ ] I accept the DCO` in the PR template when submitting it. The [ ] is an opt-in box, so you have to manually accept it.


