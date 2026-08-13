complete -c aurora-client -s d -l domain_id -r -d 'DDS domain ID (default: 123)'
complete -c aurora-client -s s -l source_id -r -d 'Nonzero source ID (default: 1234)'
complete -c aurora-client -s h -l help -d 'Show help'
complete -c aurora-client -l version -d 'Show version'

complete -c aurora-client -n 'not __fish_seen_subcommand_from state estimator motion control-group' -a state -d 'Aurora and FSM state'
complete -c aurora-client -n 'not __fish_seen_subcommand_from state estimator motion control-group' -a estimator -d 'Estimator telemetry'
complete -c aurora-client -n 'not __fish_seen_subcommand_from state estimator motion control-group' -a motion -d 'Motion state and commands'
complete -c aurora-client -n 'not __fish_seen_subcommand_from state estimator motion control-group' -a control-group -d 'Control-group state and commands'

complete -c aurora-client -n '__fish_seen_subcommand_from state; and not __fish_seen_subcommand_from get list request monitor' -a 'get list request monitor'
complete -c aurora-client -n '__fish_seen_subcommand_from estimator; and not __fish_seen_subcommand_from get monitor' -a 'get monitor'
complete -c aurora-client -n '__fish_seen_subcommand_from motion; and not __fish_seen_subcommand_from get monitor command' -a 'get monitor command'
complete -c aurora-client -n '__fish_seen_subcommand_from control-group; and not __fish_seen_subcommand_from get monitor position' -a 'get monitor position'

complete -c aurora-client -n '__fish_seen_subcommand_from monitor' -l interval -r -d 'Polling interval (default: 500ms)'
complete -c aurora-client -n '__fish_seen_subcommand_from command' -l position -r
complete -c aurora-client -n '__fish_seen_subcommand_from command' -l linear-velocity -r
complete -c aurora-client -n '__fish_seen_subcommand_from command' -l attitude -r
complete -c aurora-client -n '__fish_seen_subcommand_from command' -l angular-velocity -r
complete -c aurora-client -n '__fish_seen_subcommand_from command' -l delta-height -r
complete -c aurora-client -n '__fish_seen_subcommand_from command' -l gait-type -r
complete -c aurora-client -n '__fish_seen_subcommand_from command' -l skills -r
complete -c aurora-client -n '__fish_seen_subcommand_from command' -l duration -r
complete -c aurora-client -n '__fish_seen_subcommand_from position' -l target -r -a 'left_leg right_leg waist head left_manipulator right_manipulator left_end_effector right_end_effector'
complete -c aurora-client -n '__fish_seen_subcommand_from position' -l duration -r
complete -c aurora-client -n '__fish_seen_subcommand_from position' -l rate -r
