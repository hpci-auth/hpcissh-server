# hpcissh-server

OAuth-based ssh-server PAM module for HPCI

## Based on XSEDE oauth-ssh SERVER_0.11

- https://github.com/XSEDE/oauth-ssh/tree/SERVER_0.11
- use only server/ directory (PAM module)
  - remove client/ directory (not used for HPCI)

## for Developer

- on Almalinux 8
- `dnf install almalinux-release-devel` for libcmocka-static
- clone this repository
- Run `make develop`
- Run `make lint`
- Run `make test`
