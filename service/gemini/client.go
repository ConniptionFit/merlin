package gemini

import (
	"context"

	"google.golang.org/genai"
)

func NewClient(ctx context.Context, apiKey string) (*genai.Client, error) {
	return genai.NewClient(ctx, &genai.ClientConfig{
		APIKey:  apiKey,
		Backend: genai.BackendGeminiAPI,
	})
}

func FunctionTools() []*genai.Tool {
	falseVal := false
	return []*genai.Tool{{
		FunctionDeclarations: []*genai.FunctionDeclaration{
			{
				Name:        "set_timer",
				Description: "Set a countdown timer on the watch that will buzz when complete.",
				Parameters: &genai.Schema{
					Type: genai.TypeObject,
					Properties: map[string]*genai.Schema{
						"duration_seconds": {
							Type:        genai.TypeInteger,
							Description: "Seconds from now until the timer fires.",
						},
						"label": {
							Type:        genai.TypeString,
							Description: "Short label for the timer.",
							Nullable:    &falseVal,
						},
					},
					Required: []string{"duration_seconds"},
				},
			},
			{
				Name:        "set_timeline_pin",
				Description: "Create a timeline reminder pin on the user's phone.",
				Parameters: &genai.Schema{
					Type: genai.TypeObject,
					Properties: map[string]*genai.Schema{
						"time": {
							Type:        genai.TypeString,
							Description: "ISO8601 time for the pin.",
						},
						"title": {
							Type:        genai.TypeString,
							Description: "Pin title.",
						},
						"body": {
							Type:        genai.TypeString,
							Description: "Pin subtitle/body.",
						},
					},
					Required: []string{"time", "title"},
				},
			},
			{
				Name:        "get_weather",
				Description: "Answer a weather question using the user's approximate city context.",
				Parameters: &genai.Schema{
					Type:       genai.TypeObject,
					Properties: map[string]*genai.Schema{},
				},
			},
		},
	}}
}

func SystemPrompt(city, timezone string) string {
	return "You are an eccentric pixelated wizard inside a watch. Answer in <250 characters. " +
		"User is near " + city + ", timezone " + timezone + ". " +
		"Use set_timer for countdown requests, set_timeline_pin for future reminders, get_weather for weather questions."
}

const ModelName = "gemini-2.0-flash"
