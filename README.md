# PacMan on STM32F429I-DISC1

Embedded implementation of a simple **Pac-Man-style game** developed for the **STM32F429I-DISC1** board using STM32 HAL.

The game is displayed on the board's LCD and the player is controlled by tilting the external ADXL accelerometer over I2C. The map contains walls, collectible coins and four ghosts moving around the board.

## Features

- Pac-Man-style game running directly on STM32
- 240x320 LCD graphics using RGB565 bitmaps
- Player movement controlled by an external ADXL accelerometer over I2C
- Four independently moving ghosts
- Randomized ghost movement
- Collision detection with walls
- Coin collection
- Win condition after collecting all coins
- Lose condition after collision with a ghost
- Custom bitmap graphics for the player, ghosts, walls and coins
- Four custom maps selectable through the touchscreen interface

## Gameplay

The game board is represented as a `12 x 16` grid. Each field is rendered as a `20 x 20` pixel tile.

The player moves through the maze and collects coins while avoiding four ghosts.

The game ends when:

- all coins are collected - **win**
- the player occupies the same field as any ghost - **lose**

Walls block both player and ghost movement.

## Player Control

The player is controlled using an external **ADXL accelerometer** connected through I2C.

The X and Y acceleration values are read and the dominant axis is used to determine the movement direction:

- up
- down
- left
- right

The direction depends on the dominant X or Y tilt of the accelerometer.

## Ghost Movement

The game contains four ghosts.

Each ghost selects a random movement direction and moves only when the destination field is not blocked by a wall. This creates simple randomized enemy behaviour without requiring predefined paths.

## Bitmap Graphics

The visual elements of the game are stored as custom **RGB565 bitmaps**.

The renderer uses 20 × 20 pixel tiles and draws the correct bitmap for every field of the board.

Graphics include:

- Pac-Man sprite
- four ghost sprites
- wall tile
- coin tile
- empty field tile

Rendering is performed using `BSP_LCD_DrawBitmap_RGB565()`, which copies the RGB565 image data directly to the LCD framebuffer.

The complete screen is therefore constructed tile by tile from bitmap graphics based on the current state of the game board.

## Coins and Game State

At the beginning of the game, available board fields are filled with collectible coins.

When Pac-Man enters a field containing a coin, that field is changed to an empty field. The game continuously checks the number of remaining coins.

If no coins remain, the game changes to the **WIN** state.

The game also compares the player's coordinates with all four ghost positions. A collision with any ghost changes the state to **LOSE**.

## LCD Rendering

The STM32F429I-DISC1 display is used as the complete game interface.

For every game update, the renderer scans the board and chooses the correct bitmap according to the current state of each field.

Player and ghost positions are rendered with priority over the underlying map tile, so moving objects are displayed at their current coordinates while the board state is preserved separately.

The game loop repeatedly performs:

1. player input acquisition,
2. board rendering,
3. player movement,
4. ghost movement,
5. game-state verification.

A delay between iterations controls the overall game speed.

## Project Structure

```text
Core/
├── Inc/
│   ├── icons.h
│   └── ...
└── Src/
    ├── main.c
    └── ...

Drivers/        - STM32 HAL, CMSIS and STM32F429I-DISC1 BSP drivers
Middlewares/    - middleware components
USB_HOST/       - generated USB Host support
Utilities/      - fonts and utility resources

PacMan.ioc                  - STM32CubeMX configuration
STM32F429ZITX_FLASH.ld      - Flash linker script
STM32F429ZITX_RAM.ld        - RAM linker script
```

`icons.h` contains the bitmap data used for the game graphics, while `main.c` contains the main game logic, movement handling, map initialization and LCD rendering.


## Development Environment

The project was created for **STM32F429I-DISC1** and developed using **STM32CubeIDE** and STM32 HAL libraries.
