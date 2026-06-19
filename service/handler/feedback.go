package handler

import (
	"encoding/json"
	"io"
	"log"
	"net/http"
	"strings"
)

type feedbackRequest struct {
	Prompt         string `json:"prompt"`
	Feedback       bool   `json:"feedback"`
	FeedbackText   string `json:"feedback_text"`
	OriginalPrompt string `json:"original_prompt"`
	OriginalReply  string `json:"original_reply"`
	Timezone       string `json:"timezone"`
}

func HandleFeedback(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}

	if strings.TrimSpace(r.Header.Get("X-Gemini-Key")) == "" {
		http.Error(w, "missing X-Gemini-Key", http.StatusUnauthorized)
		return
	}

	body, err := io.ReadAll(io.LimitReader(r.Body, 1<<20))
	if err != nil {
		http.Error(w, "invalid body", http.StatusBadRequest)
		return
	}

	var req feedbackRequest
	if err := json.Unmarshal(body, &req); err != nil {
		http.Error(w, "invalid json", http.StatusBadRequest)
		return
	}

	feedback := req.FeedbackText
	if feedback == "" {
		feedback = req.Prompt
	}

	log.Printf("merlin feedback prompt=%q reply=%q feedback=%q tz=%s",
		req.OriginalPrompt, req.OriginalReply, feedback, req.Timezone)

	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(map[string]bool{"ok": true})
}
