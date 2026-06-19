package handler

import (
	"encoding/json"
	"errors"
	"io"
	"net/http"
	"strings"

	"github.com/merlin-assistant/service/gemini"
	"github.com/merlin-assistant/service/geo"
	"google.golang.org/genai"
)

type queryRequest struct {
	Prompt   string `json:"prompt"`
	Feedback bool   `json:"feedback"`
	Timezone string `json:"timezone"`
}

func HandleQuery(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}

	apiKey := strings.TrimSpace(r.Header.Get("X-Gemini-Key"))
	if apiKey == "" {
		http.Error(w, "missing X-Gemini-Key", http.StatusUnauthorized)
		return
	}

	body, err := io.ReadAll(io.LimitReader(r.Body, 1<<20))
	if err != nil {
		http.Error(w, "invalid body", http.StatusBadRequest)
		return
	}

	var req queryRequest
	if err := json.Unmarshal(body, &req); err != nil || strings.TrimSpace(req.Prompt) == "" {
		http.Error(w, "invalid prompt", http.StatusBadRequest)
		return
	}

	if req.Timezone == "" {
		req.Timezone = "UTC"
	}

	city := geo.RedactLocation(r.Context(), r.Header.Get("X-Latitude"), r.Header.Get("X-Longitude"))
	client, err := gemini.NewClient(r.Context(), apiKey)
	if err != nil {
		http.Error(w, "gemini client error", http.StatusInternalServerError)
		return
	}

	temperature := float32(0.5)
	resp, err := client.Models.GenerateContent(r.Context(), gemini.ModelName, genai.Text(req.Prompt), &genai.GenerateContentConfig{
		SystemInstruction: &genai.Content{
			Parts: []*genai.Part{{Text: gemini.SystemPrompt(city, req.Timezone)}},
		},
		Temperature: &temperature,
		Tools:       gemini.FunctionTools(),
	})
	if err != nil {
		http.Error(w, "gemini request failed", http.StatusBadGateway)
		return
	}

	result, err := resultFromResponse(resp)
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadGateway)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(result)
}

func resultFromResponse(resp *genai.GenerateContentResponse) (gemini.QueryResult, error) {
	if resp == nil {
		return gemini.QueryResult{}, errors.New("empty gemini response")
	}

	if calls := resp.FunctionCalls(); len(calls) > 0 {
		call := calls[0]
		argsJSON, _ := json.Marshal(call.Args)
		return gemini.ParseFunctionResult(call.Name, argsJSON, resp.Text()), nil
	}

	reply := strings.TrimSpace(resp.Text())
	if reply == "" {
		reply = "The wizard ponders silently."
	}
	return gemini.QueryResult{Reply: gemini.TruncateReply(reply), Intent: "General"}, nil
}
