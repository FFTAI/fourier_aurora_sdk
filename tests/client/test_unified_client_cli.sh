#!/usr/bin/env bash
set -euo pipefail

client="${1:?aurora-client path is required}"
completion_dir="${2:?completion directory is required}"

expect_status() {
    local expected="$1"
    shift
    set +e
    "$@" >/tmp/aurora-client-test.out 2>/tmp/aurora-client-test.err
    local actual=$?
    set -e
    if [[ "${actual}" -ne "${expected}" ]]; then
        echo "expected exit ${expected}, got ${actual}: $*" >&2
        sed -n '1,120p' /tmp/aurora-client-test.out >&2
        sed -n '1,120p' /tmp/aurora-client-test.err >&2
        exit 1
    fi
}

expect_status 0 "${client}"
grep -Fq 'state          Aurora and FSM state' /tmp/aurora-client-test.out
expect_status 0 "${client}" --version
grep -Eq '^aurora-client [0-9]+\.[0-9]+\.[0-9]+$' /tmp/aurora-client-test.out
expect_status 0 "${client}" state
grep -Fq 'set <state-id>' /tmp/aurora-client-test.out
expect_status 0 "${client}" state set --help
grep -Fq 'state set <state-id>' /tmp/aurora-client-test.out
grep -Fq -- '--force' /tmp/aurora-client-test.out
expect_status 0 "${client}" motion set --help
grep -Fq -- '--linear-velocity' /tmp/aurora-client-test.out
expect_status 0 "${client}" control-group set --help
grep -Fq -- '--target' /tmp/aurora-client-test.out
expect_status 0 "${client}" state monitor --domain_id 7 --source_name read-client --help
grep -Fq 'minimum: 10ms' /tmp/aurora-client-test.out
if grep -Fq 'domain_id=' /tmp/aurora-client-test.err; then
    echo "help unexpectedly initialized the client" >&2
    exit 1
fi

expect_status 2 "${client}" state monitor --interval 9ms
grep -Fq 'at least 10ms' /tmp/aurora-client-test.err
expect_status 2 "${client}" state monitor --interval 500 --interval 1s
grep -Fq 'specified more than once' /tmp/aurora-client-test.err
expect_status 2 "${client}" -d 7 state get --domain_id 8
grep -Fq 'specified more than once' /tmp/aurora-client-test.err
expect_status 2 "${client}" state get --source_name 'invalid name'
grep -Fq '[A-Za-z0-9._-]{1,64}' /tmp/aurora-client-test.err
expect_status 2 "${client}" state get --source_id 42
grep -Fq 'does not accept command options' /tmp/aurora-client-test.err
expect_status 1 "${client}" -d 231 state get
grep -Eq '^domain_id=231 source_name=aurora-client-[0-9]{4}$' /tmp/aurora-client-test.err
expect_status 2 "${client}" estimator monitor --interval 1minute
grep -Fq 'positive duration in ms or s' /tmp/aurora-client-test.err
expect_status 2 "${client}" motion set
grep -Fq 'requires at least one command field' /tmp/aurora-client-test.err
expect_status 2 "${client}" control-group set --duration 1
grep -Fq 'requires at least one --target' /tmp/aurora-client-test.err
expect_status 2 "${client}" state request 13
grep -Fq 'unknown state command: request' /tmp/aurora-client-test.err
expect_status 2 "${client}" state set 13 --force --force
grep -Fq -- '--force specified more than once' /tmp/aurora-client-test.err
expect_status 2 "${client}" motion command --position 0 0 0
grep -Fq 'unknown motion command: command' /tmp/aurora-client-test.err
expect_status 2 "${client}" control-group position --target head 0 0
grep -Fq 'unknown control-group command: position' /tmp/aurora-client-test.err

bash -n "${completion_dir}/aurora-client.bash"
source "${completion_dir}/aurora-client.bash"
COMP_WORDS=(aurora-client mo)
COMP_CWORD=1
_aurora_client
[[ " ${COMPREPLY[*]} " == *' motion '* ]]
COMP_WORDS=(aurora-client state m)
COMP_CWORD=2
_aurora_client
[[ " ${COMPREPLY[*]} " == *' monitor '* ]]
COMP_WORDS=(aurora-client state s)
COMP_CWORD=2
_aurora_client
[[ " ${COMPREPLY[*]} " == *' set '* ]]
COMP_WORDS=(aurora-client state set --fo)
COMP_CWORD=3
_aurora_client
[[ " ${COMPREPLY[*]} " == *' --force '* ]]
COMP_WORDS=(aurora-client motion set --lin)
COMP_CWORD=3
_aurora_client
[[ " ${COMPREPLY[*]} " == *' --linear-velocity '* ]]
COMP_WORDS=(aurora-client control-group set --ta)
COMP_CWORD=3
_aurora_client
[[ " ${COMPREPLY[*]} " == *' --target '* ]]

if command -v zsh >/dev/null 2>&1; then
    zsh -n "${completion_dir}/_aurora-client"
fi
if command -v fish >/dev/null 2>&1; then
    fish -n "${completion_dir}/aurora-client.fish"
fi
