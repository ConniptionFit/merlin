package gemini

import (
	"encoding/json"
	"strings"
	"time"

	"github.com/google/uuid"
)

type QueryResult struct {
	Reply        string          `json:"reply"`
	Intent       string          `json:"intent"`
	Timer        *TimerAction    `json:"timer,omitempty"`
	TimelinePin  *TimelinePin    `json:"timeline_pin,omitempty"`
}

type TimerAction struct {
	DurationSeconds int32  `json:"duration_seconds"`
	Label           string `json:"label,omitempty"`
}

type TimelinePin struct {
	ID     string         `json:"id"`
	Time   string         `json:"time"`
	Layout TimelineLayout `json:"layout"`
}

type TimelineLayout struct {
	Type     string `json:"type"`
	Title    string `json:"title"`
	Subtitle string `json:"subtitle,omitempty"`
}

type setTimerArgs struct {
	DurationSeconds int32  `json:"duration_seconds"`
	Label           string `json:"label"`
}

type setTimelinePinArgs struct {
	Time  string `json:"time"`
	Title string `json:"title"`
	Body  string `json:"body"`
}

func TruncateReply(text string) string {
	text = strings.TrimSpace(text)
	if len(text) <= 250 {
		return text
	}
	return text[:250]
}

func BuildTimelinePin(args setTimelinePinArgs) *TimelinePin {
	pinTime := args.Time
	if pinTime == "" {
		pinTime = time.Now().UTC().Add(time.Hour).Format(time.RFC3339)
	}
	return &TimelinePin{
		ID:   uuid.NewString(),
		Time: pinTime,
		Layout: TimelineLayout{
			Type:     "generic",
			Title:    args.Title,
			Subtitle: args.Body,
		},
	}
}

func ParseFunctionResult(name string, argsJSON []byte, modelText string) QueryResult {
	result := QueryResult{
		Reply:  TruncateReply(modelText),
		Intent: "General",
	}

	switch name {
	case "set_timer":
		var args setTimerArgs
		if err := json.Unmarshal(argsJSON, &args); err == nil && args.DurationSeconds > 0 {
			result.Intent = "SetTimer"
			result.Timer = &TimerAction{
				DurationSeconds: args.DurationSeconds,
				Label:           args.Label,
			}
			if result.Reply == "" {
				result.Reply = "Timer set, mortal."
			}
		}
	case "set_timeline_pin":
		var args setTimelinePinArgs
		if err := json.Unmarshal(argsJSON, &args); err == nil {
			result.Intent = "SetTimelinePin"
			result.TimelinePin = BuildTimelinePin(args)
			if result.Reply == "" {
				result.Reply = "A reminder spell is cast."
			}
		}
	case "get_weather":
		result.Intent = "Weather"
		if result.Reply == "" {
			result.Reply = "The clouds whisper, but say little."
		}
	}

	return result
}
