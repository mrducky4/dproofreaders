<?php
// Invoking this file via include() causes the "image & text & buttons" parts
// of the proofing interface (i.e., everything but the "AEIOU+" panel)
// to be sent to the user.

include_once('proof_frame_std.inc');
include_once('proof_frame_enh.inc');

function echo_proof_frame(PPage $ppage): void
{
    $user = User::load_current();

    $interface_type = $user->profile->i_type;
    switch ($interface_type) {
        case 0:
            // standard interface
            echo_proof_frame_std($ppage);
            break;

        case 1:
            // enhanced interface
            echo_proof_frame_enh($ppage);
            break;

        default:
            throw new UnexpectedValueException("unknown i_type: '$interface_type'");
    }
}
