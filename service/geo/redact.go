package geo

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"strconv"
	"strings"
	"time"
)

type nominatimResponse struct {
	Address struct {
		City        string `json:"city"`
		Town        string `json:"town"`
		Village     string `json:"village"`
		County      string `json:"county"`
		State       string `json:"state"`
		Country     string `json:"country"`
		Municipality string `json:"municipality"`
	} `json:"address"`
}

func RedactLocation(ctx context.Context, latHeader, lonHeader string) string {
	lat, errLat := strconv.ParseFloat(strings.TrimSpace(latHeader), 64)
	lon, errLon := strconv.ParseFloat(strings.TrimSpace(lonHeader), 64)
	if errLat != nil || errLon != nil {
		return "unknown location"
	}

	reqCtx, cancel := context.WithTimeout(ctx, 4*time.Second)
	defer cancel()

	url := fmt.Sprintf("https://nominatim.openstreetmap.org/reverse?format=jsonv2&lat=%f&lon=%f&zoom=10", lat, lon)
	req, err := http.NewRequestWithContext(reqCtx, http.MethodGet, url, nil)
	if err != nil {
		return "unknown location"
	}
	req.Header.Set("User-Agent", "MerlinPebbleAssistant/1.0")

	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		return "unknown location"
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return "unknown location"
	}

	var payload nominatimResponse
	if err := json.NewDecoder(resp.Body).Decode(&payload); err != nil {
		return "unknown location"
	}

	for _, candidate := range []string{
		payload.Address.City,
		payload.Address.Town,
		payload.Address.Village,
		payload.Address.Municipality,
		payload.Address.County,
	} {
		if strings.TrimSpace(candidate) != "" {
			return candidate
		}
	}

	if payload.Address.State != "" {
		return payload.Address.State
	}
	return "unknown location"
}
