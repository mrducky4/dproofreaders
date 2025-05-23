<?php
declare(strict_types=1);

class ProofProjectPage extends LPage
{
    public function __construct(ProofProject $proof_project, ProjectPage $project_page)
    {
        parent::__construct($proof_project->project, $project_page->page_name, $project_page->page_state, 0);
    }

    public function pp_checkout(string $user): array
    {
        parent::checkout($user);
        return $this->render_page_data();
    }

    public function attempt_checkin(string $page_text): array
    {
        global $pguser;

        $this->project->validate_text($page_text);
        // see comment in LPage.inc
        [$saved, $dpl_reached] = $this->attemptSaveAsDone($page_text, $pguser);

        if (!$saved) {
            $sentence = sprintf(
                _("Your page was not checked-in because you have already reached the daily page limit for %s. You can save it and will be able to check it in after server midnight."),
                $this->round->id
            );
            throw new DailyLimitExceededException($sentence);
        }

        if (!$dpl_reached) {
            $sentence = "";
            $status_code = 0;
        } else {
            $sentence = sprintf(
                _("Your page has been checked-in. However, you have now reached the daily page limit for %s."),
                $this->round->id
            );
            $status_code = 1;
        }
        return ["message" => $sentence, "status" => $status_code];
    }

    public function save(string $page_text): array
    {
        global $pguser;
        $this->project->validate_text($page_text);
        $this->saveAsInProgress($page_text, $pguser);
        return $this->get_page_text_data();
    }

    public function save_and_revert(string $page_text): array
    {
        global $pguser;
        $this->project->validate_text($page_text);
        $this->saveAsInProgress($page_text, $pguser);
        $this->revertToOriginal();
        return $this->get_page_text_data();
    }

    public function pp_resume_page(): array
    {
        global $pguser;

        parent::resume_page($pguser);
        return $this->render_page_data();
    }

    private function render_page_data(): array
    {
        return
            $this->get_page_text_data() +
            [
                "pagenum" => $this->get_filename(),
                "round_info" => $this->get_info(),
                "pagename" => $this->imagefile,
                "image_url" => "{$this->project->url}/{$this->imagefile}",
                "language_direction" => lang_direction(langcode2_for_langname($this->project->languages[0])),
            ];
    }

    public function get_page_text_data(): array
    {
        return [
            "text" => $this->get_text(),
            "pagestate" => $this->page_state,
            "saved" => $this->can_be_reverted_to_last_save(),
        ];
    }

    public function wc_report(array $accepted_words): void
    {
        global $pguser;

        _Page_require(
            $this->projectid,
            $this->imagefile,
            [$this->round->page_out_state, $this->round->page_temp_state],
            $this->round->user_column_name,
            $pguser,
            'report'
        );
        save_wordcheck_event(
            $this->projectid,
            $this->round->id,
            $this->imagefile,
            $pguser,
            $accepted_words
        );
    }
}

class ProofProject
{
    public $project;
    public $round;

    public function __construct(Project $project)
    {
        global $pguser;

        $project_round = $project->get_project_available_round();
        $project_round->validate_user_can_access($pguser);

        $this->project = $project;
        $this->round = $project_round;
    }

    public function checkout(): array
    {
        global $pguser;

        [$page_name, $state] = get_available_proof_page_array($this->project, $this->round, $pguser);
        $project_page = new ProjectPage($page_name, $state);
        $proof_project_page = new ProofProjectPage($this, $project_page);

        return $proof_project_page->pp_checkout($pguser);
    }
}
