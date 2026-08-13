_aurora_client()
{
    local cur area='' command='' word=''
    cur="${COMP_WORDS[COMP_CWORD]}"

    for word in "${COMP_WORDS[@]:1}"; do
        case "${word}" in
            state|estimator|motion|control-group)
                [[ -z "${area}" ]] && area="${word}"
                ;;
            get|list|request|monitor|command|position)
                [[ -n "${area}" && -z "${command}" ]] && command="${word}"
                ;;
        esac
    done

    if [[ -z "${area}" ]]; then
        COMPREPLY=( $(compgen -W \
            'state estimator motion control-group -d --domain_id -s --source_id -h --help --version' \
            -- "${cur}") )
        return
    fi
    if [[ -z "${command}" ]]; then
        case "${area}" in
            state) word='get list request monitor -d --domain_id -s --source_id -h --help' ;;
            estimator) word='get monitor -d --domain_id -s --source_id -h --help' ;;
            motion) word='get monitor command -d --domain_id -s --source_id -h --help' ;;
            control-group) word='get monitor position -d --domain_id -s --source_id -h --help' ;;
        esac
        COMPREPLY=( $(compgen -W "${word}" -- "${cur}") )
        return
    fi

    word='-d --domain_id -s --source_id -h --help'
    case "${command}" in
        monitor) word="${word} --interval" ;;
        command)
            word="${word} --position --linear-velocity --attitude --angular-velocity --delta-height --gait-type --skills --duration"
            ;;
        position) word="${word} --target --duration --rate" ;;
    esac
    if [[ "${COMP_WORDS[COMP_CWORD-1]}" == '--target' ]]; then
        word='left_leg right_leg waist head left_manipulator right_manipulator left_end_effector right_end_effector'
    fi
    COMPREPLY=( $(compgen -W "${word}" -- "${cur}") )
}

complete -F _aurora_client aurora-client
