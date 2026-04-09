-- Rename cinematic_camera coordinate columns to match server source

ALTER TABLE `cinematic_camera`
    CHANGE COLUMN `Origin1` `OriginX` FLOAT NOT NULL DEFAULT '0',
    CHANGE COLUMN `Origin2` `OriginY` FLOAT NOT NULL DEFAULT '0',
    CHANGE COLUMN `Origin3` `OriginZ` FLOAT NOT NULL DEFAULT '0';